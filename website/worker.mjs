// Dependency-free Sites Worker. The build appends its immutable asset catalogue.
export const NEWSLETTER_CONSENT_VERSION = '2026-11-23-newsletter-v1';

const encoder = new TextEncoder();
const DAY = 86_400_000;
const GENERIC_SUBSCRIBE = 'If this address can receive updates, a confirmation email will arrive shortly.';

export function createSiteWorker(config, deps = {}) {
  const cryptoApi = deps.crypto || globalThis.crypto;
  const transport = deps.fetch || ((...args) => globalThis.fetch(...args));
  const now = () => new Date(deps.now ? deps.now() : Date.now());
  const origin = new URL(config.canonicalOrigin).origin;
  const newsletterUrl = config.newsletterUrl ? new URL(config.newsletterUrl).href : null;
  if (newsletterUrl && (!newsletterUrl.startsWith('https://') || new URL(newsletterUrl).origin === origin)) throw new Error('Expected an external HTTPS newsletter page');
  const assets = config.assets || {};
  const decoded = new Map();let decodedBytes=0;
  function assetBytes(path,asset){
    if(decoded.has(path)){const bytes=decoded.get(path);decoded.delete(path);decoded.set(path,bytes);return bytes;}
    let bytes;
    if(typeof Uint8Array.fromBase64==='function')bytes=Uint8Array.fromBase64(asset.data);
    else {const binary=atob(asset.data);bytes=new Uint8Array(binary.length);for(let n=0;n<binary.length;n++)bytes[n]=binary.charCodeAt(n);}
    if(bytes.byteLength!==Number(asset.bytes))throw new HttpError(500,'This file is temporarily unavailable.');
    if(bytes.byteLength<=8*1024*1024){while(decodedBytes+bytes.byteLength>8*1024*1024&&decoded.size){const oldest=decoded.keys().next().value;decodedBytes-=decoded.get(oldest).byteLength;decoded.delete(oldest);}decoded.set(path,bytes);decodedBytes+=bytes.byteLength;}
    return bytes;
  }
  const byId = new Map();
  const byPath = new Map();
  for (const file of [...(config.catalog?.releases || []), ...(config.catalog?.legacy || [])]) {
    if (!file.id || !file.path) continue;
    const path = file.path.startsWith('/') ? file.path : `/${file.path}`;
    if (byId.has(file.id) && byId.get(file.id).path !== path) throw new Error('Duplicate download ID');
    byId.set(file.id, { ...file, path });
    if (!byPath.has(path)) byPath.set(path, file.id);
  }

  async function hash(value) {
    const digest = await cryptoApi.subtle.digest('SHA-256', encoder.encode(value));
    return [...new Uint8Array(digest)].map(value => value.toString(16).padStart(2, '0')).join('');
  }

  async function isAdmin(request, env) {
    const expected = env.ADMIN_ACCESS_HASH;
    const bearer = /^Bearer ([^\s]+)$/i.exec(request.headers.get('Authorization') || '')?.[1];
    if (!/^[a-f0-9]{64}$/i.test(expected || '') || !bearer || bearer.length > 2048) return false;
    const actual = await hash(bearer);
    let difference = 0;
    const normalized = expected.toLowerCase();
    for (let index = 0; index < 64; index++) difference |= actual.charCodeAt(index) ^ normalized.charCodeAt(index);
    return difference === 0;
  }

  function token() {
    const bytes = cryptoApi.getRandomValues(new Uint8Array(32));
    return btoa(String.fromCharCode(...bytes)).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
  }

  function newsletterSettings(env) {
    if(env.NEWSLETTER_ENABLED!=='true')return null;
    const provider = String(env.NEWSLETTER_PROVIDER || '').trim().toLowerCase();
    const key = String(env.NEWSLETTER_API_KEY || '').trim();
    const from = normalizeEmail(env.NEWSLETTER_FROM_EMAIL);
    if (!['brevo', 'resend'].includes(provider) || !key || !from) return null;
    const name = String(env.NEWSLETTER_FROM_NAME || config.title || config.project).replace(/[\r\n<>]/g, '').slice(0, 100);
    return { provider, key, from, name };
  }

  async function sendConfirmation(settings, email, confirmation, unsubscribe) {
    // Fragments keep bearer tokens out of HTTP request URLs and referrers.
    const confirmLink = `${origin}/newsletter/#confirm=${confirmation}`;
    const unsubscribeLink = `${origin}/newsletter/#unsubscribe=${unsubscribe}`;
    const title = String(config.title || config.project);
    const subject = `${title}: Newsletter-Anmeldung bestätigen`;
    const text = `Du möchtest Neuigkeiten von ${title} erhalten.\n\nBestätige deine Anmeldung innerhalb von 24 Stunden:\n${confirmLink}\n\nFalls du dich nicht angemeldet hast, ignoriere diese Nachricht.\nAbmelden:\n${unsubscribeLink}`;
    const html = `<p>Du möchtest Neuigkeiten von ${escapeHtml(title)} erhalten.</p><p><a href="${confirmLink}">Anmeldung bestätigen</a> (innerhalb von 24 Stunden).</p><p>Falls du dich nicht angemeldet hast, ignoriere diese Nachricht.</p><p><a href="${unsubscribeLink}">Abmelden</a></p>`;
    const resend = settings.provider === 'resend';
    const response = await transport(resend ? 'https://api.resend.com/emails' : 'https://api.brevo.com/v3/smtp/email', {
      method: 'POST',
      headers: resend
        ? { 'Content-Type': 'application/json', Authorization: `Bearer ${settings.key}` }
        : { 'Content-Type': 'application/json', 'api-key': settings.key },
      body: JSON.stringify(resend
        ? { from: settings.name ? `${settings.name} <${settings.from}>` : settings.from, to: [email], subject, text, html }
        : { sender: { email: settings.from, name: settings.name }, to: [{ email }], subject, textContent: text, htmlContent: html }),
      signal: AbortSignal.timeout(10_000),
    });
    if (!response.ok) throw new Error('Mail transport unavailable');
    await response.body?.cancel();
  }

  async function subscribe(request, env) {
    requireOrigin(request);
    const body = await readJson(request);
    if (body.consent !== true || body.consentVersion !== NEWSLETTER_CONSENT_VERSION) {
      throw new HttpError(400, 'Please explicitly agree to receive the newsletter.');
    }
    const email = normalizeEmail(body.email);
    if (!email) throw new HttpError(400, 'Enter a valid email address.');
    const settings = newsletterSettings(env);
    if (!settings) throw new HttpError(503, 'Newsletter signup is not available yet.');
    if (typeof body.company !== 'string') throw new HttpError(400, 'Invalid signup form.');
    if (body.company) return json({ ok: true, message: GENERIC_SUBSCRIBE }, 202);
    if (!env.DB) throw new HttpError(503, 'Newsletter signup is temporarily unavailable.');

    const timestamp = now();
    const iso = timestamp.toISOString();
    const cooldown = new Date(timestamp.getTime() - 15 * 60_000).toISOString();
    const minute = iso.slice(0, 16);
    try {
      // These are bounded data-retention operations, never schema changes.
      await env.DB.batch([
        env.DB.prepare("DELETE FROM newsletter_subscribers WHERE status = 'pending' AND updated_at < ?").bind(new Date(timestamp.getTime() - 7 * DAY).toISOString()),
        env.DB.prepare('DELETE FROM newsletter_send_budget WHERE minute < ?').bind(new Date(timestamp.getTime() - DAY).toISOString().slice(0, 16)),
      ]);
      const existing = await env.DB.prepare('SELECT status, updated_at FROM newsletter_subscribers WHERE email = ?').bind(email).first();
      if (existing?.status === 'confirmed' || (existing && existing.updated_at > cooldown)) {
        return json({ ok: true, message: GENERIC_SUBSCRIBE }, 202);
      }
      const budget = await env.DB.prepare(`INSERT INTO newsletter_send_budget (minute, requests) VALUES (?, 1)
        ON CONFLICT(minute) DO UPDATE SET requests = requests + 1 WHERE requests < 5 RETURNING requests`).bind(minute).first();
      if (!budget) throw new HttpError(429, 'Newsletter signup is busy. Please try again later.');
      const confirmation = token();
      const unsubscribe = token();
      const confirmationHash = await hash(confirmation);
      const unsubscribeHash = await hash(unsubscribe);
      // RETURNING claims a resend atomically, including concurrent requests.
      const claimed = await env.DB.prepare(`INSERT INTO newsletter_subscribers
        (email, status, consent_version, consented_at, confirmed_at, confirmation_hash, confirmation_expires_at, unsubscribe_hash, mail_sent_at, updated_at)
        VALUES (?, 'pending', ?, ?, NULL, ?, ?, ?, NULL, ?)
        ON CONFLICT(email) DO UPDATE SET status = 'pending', consent_version = excluded.consent_version,
          consented_at = excluded.consented_at, confirmed_at = NULL, confirmation_hash = excluded.confirmation_hash,
          confirmation_expires_at = excluded.confirmation_expires_at, unsubscribe_hash = excluded.unsubscribe_hash,
          mail_sent_at = NULL, updated_at = excluded.updated_at
        WHERE newsletter_subscribers.status <> 'confirmed' AND newsletter_subscribers.updated_at <= ?
        RETURNING email`).bind(email, NEWSLETTER_CONSENT_VERSION, iso, confirmationHash,
          new Date(timestamp.getTime() + DAY).toISOString(), unsubscribeHash, iso, cooldown).first();
      if (!claimed) return json({ ok: true, message: GENERIC_SUBSCRIBE }, 202);
      await sendConfirmation(settings, email, confirmation, unsubscribe);
      await env.DB.prepare('UPDATE newsletter_subscribers SET mail_sent_at = ? WHERE email = ? AND confirmation_hash = ?')
        .bind(iso, email, confirmationHash).run();
      return json({ ok: true, message: GENERIC_SUBSCRIBE }, 202);
    } catch (error) {
      if (error instanceof HttpError) throw error;
      // Never expose provider responses, addresses, or tokens in errors/logs.
      throw new HttpError(503, 'Newsletter confirmation is temporarily unavailable. Please try again later.');
    }
  }

  async function tokenAction(request, env, action) {
    requireOrigin(request);
    const body = await readJson(request);
    if (typeof body.token !== 'string' || !/^[A-Za-z0-9_-]{43}$/.test(body.token)) throw new HttpError(400, 'This link is invalid or has expired.');
    if (!env.DB) throw new HttpError(503, 'Newsletter settings are temporarily unavailable.');
    const tokenHash = await hash(body.token);
    const iso = now().toISOString();
    let result;
    try {
      result = action === 'confirm'
        ? await env.DB.prepare(`UPDATE newsletter_subscribers SET status = 'confirmed', confirmed_at = ?,
            confirmation_hash = NULL, confirmation_expires_at = NULL, updated_at = ?
            WHERE status = 'pending' AND confirmation_hash = ? AND confirmation_expires_at >= ? RETURNING email`)
          .bind(iso, iso, tokenHash, iso).first()
        : await env.DB.prepare(`UPDATE newsletter_subscribers SET status = 'unsubscribed', confirmation_hash = NULL,
            confirmation_expires_at = NULL, updated_at = ? WHERE unsubscribe_hash = ? RETURNING email`)
          .bind(iso, tokenHash).first();
    } catch {
      throw new HttpError(503, 'Newsletter settings are temporarily unavailable.');
    }
    if (!result) throw new HttpError(400, 'This link is invalid or has expired.');
    return json({ ok: true, message: action === 'confirm' ? 'Your subscription is confirmed.' : 'You have been unsubscribed.' });
  }

  async function stats(url, env) {
    if (!env.DB) throw new HttpError(503, 'Statistics are temporarily unavailable.');
    const today = now().toISOString().slice(0, 10);
    const oldest = new Date(Date.parse(today) - 89 * DAY).toISOString().slice(0, 10);
    let days = 30;
    if (url.searchParams.has('days')) {
      const input = url.searchParams.get('days');
      if (!/^[1-9]\d*$/.test(input || '') || Number(input) > 90) throw new HttpError(400, 'Choose between 1 and 90 days.');
      days = Number(input);
    }
    let to = url.searchParams.get('to') || today;
    if (!isDate(to)) throw new HttpError(400, 'Use dates in YYYY-MM-DD format.');
    to = to > today ? today : to;
    let from = url.searchParams.get('from') || new Date(Date.parse(to) - (days - 1) * DAY).toISOString().slice(0, 10);
    if (!isDate(from) || !isDate(to)) throw new HttpError(400, 'Use dates in YYYY-MM-DD format.');
    from = from < oldest ? oldest : from;
    if (from > to) throw new HttpError(400, 'Choose a valid date range within the last 90 days.');
    try {
      const [files, daily, allTime, subscribers] = await Promise.all([
        env.DB.prepare('SELECT file_id, SUM(requests) AS requests FROM download_daily WHERE day >= ? AND day <= ? GROUP BY file_id ORDER BY requests DESC, file_id').bind(from, to).all(),
        env.DB.prepare('SELECT day, file_id, requests FROM download_daily WHERE day >= ? AND day <= ? ORDER BY day DESC, file_id').bind(from, to).all(),
        env.DB.prepare('SELECT COALESCE(SUM(requests), 0) AS requests FROM download_daily').first(),
        env.DB.prepare('SELECT status, COUNT(*) AS count FROM newsletter_subscribers GROUP BY status').all(),
      ]);
      const newsletter = { pending: 0, confirmed: 0, unsubscribed: 0 };
      for (const row of subscribers.results || []) if (Object.hasOwn(newsletter, row.status)) newsletter[row.status] = Number(row.count);
      const byFile = (files.results || []).map(row => ({ file_id: row.file_id, requests: Number(row.requests) }));
      return json({ project: config.project, from, to,
        downloads: { total: byFile.reduce((sum, row) => sum + row.requests, 0), allTimeTotal: Number(allTime?.requests || 0), byFile, byDay: daily.results || [] }, newsletter });
    } catch {
      throw new HttpError(503, 'Statistics are temporarily unavailable.');
    }
  }

  async function newsletterCsv(env) {
    if (!env.DB) throw new HttpError(503, 'Newsletter export is temporarily unavailable.');
    let rows;
    try {
      rows = (await env.DB.prepare("SELECT email, consent_version, consented_at, confirmed_at FROM newsletter_subscribers WHERE status = 'confirmed' ORDER BY email LIMIT 10001").all()).results || [];
    } catch {
      throw new HttpError(503, 'Newsletter export is temporarily unavailable.');
    }
    if (rows.length > 10_000) throw new HttpError(413, 'Use the database viewer to export this larger list.');
    const columns = ['email', 'consent_version', 'consented_at', 'confirmed_at'];
    const body = [columns, ...rows.map(row => columns.map(column => row[column] ?? ''))]
      .map(row => row.map(csvCell).join(',')).join('\r\n') + '\r\n';
    return new Response(body, { headers: { 'Content-Type': 'text/csv; charset=utf-8', 'Cache-Control': 'private, no-store',
      'Content-Disposition': `attachment; filename="${String(config.project).replace(/[^a-z0-9_-]/gi, '')}-newsletter.csv"` } });
  }

  async function staticFile(request, url, env) {
    let path = safePath(url.pathname);
    let fileId;
    const get = /^\/get\/([^/]+)$/.exec(path);
    if (get) {
      const file = byId.get(get[1]);
      if (!file) throw new HttpError(404, 'Download not found.');
      path = file.path;
      fileId = file.id;
    }
    if (!Object.hasOwn(assets, path)) {
      const index = `${path.endsWith('/') ? path : `${path}/`}index.html`;
      if (Object.hasOwn(assets, index)) {
        // Keep relative links correct when a visitor omits a directory slash.
        if (!path.endsWith('/')) return new Response(null, { status: 308, headers: { Location: `${url.pathname}/${url.search}`, 'Cache-Control': 'public, max-age=300' } });
        path = index;
      }
      else throw new HttpError(404, 'Page not found.');
    }
    fileId ||= byPath.get(path);
    const asset = assets[path];
    const size = Number(asset.bytes);
    const etag = `"${asset.sha256}"`;
    const headers = new Headers({ 'Content-Type': asset.type || 'application/octet-stream',
      'Content-Length': String(size), ETag: etag, 'Accept-Ranges': 'bytes',
      'Cache-Control': fileId ? 'private, no-store' : path.endsWith('.html') || path.endsWith('/download-catalog.json') ? 'no-cache' : 'public, max-age=300' });
    if (fileId) {
      const filename = path.slice(path.lastIndexOf('/') + 1).replace(/[\r\n"\\]/g, '_');
      headers.set('Content-Disposition', `attachment; filename="${filename}"`);
    }
    if (matchesEtag(request.headers.get('If-None-Match'), etag)) {
      headers.delete('Content-Length');
      return new Response(null, { status: 304, headers });
    }
    if (request.method === 'HEAD') return new Response(null, { headers });
    const rangeHeader = request.headers.get('Range');
    let range;
    if (rangeHeader && (!request.headers.has('If-Range') || request.headers.get('If-Range') === etag)) {
      range = byteRange(rangeHeader, size);
      if (!range) {
        headers.set('Content-Range', `bytes */${size}`);
        headers.set('Content-Length', '0');
        return new Response(null, { status: 416, headers });
      }
    }
    const bytes = assetBytes(path,asset);
    if (range) {
      headers.set('Content-Range', `bytes ${range.start}-${range.end}/${size}`);
      headers.set('Content-Length', String(range.end - range.start + 1));
      return new Response(bytes.subarray(range.start, range.end + 1), { status: 206, headers });
    }
    const prefetch = [request.headers.get('Purpose'), request.headers.get('Sec-Purpose')]
      .some(value => /(?:^|[\s;,])(?:prefetch|prerender)(?:$|[\s;,])/i.test(value || ''));
    if (fileId && !rangeHeader && !prefetch) {
      const verification = request.headers.get('X-Download-Verification') === '1' && await isAdmin(request, env);
      if (!verification && env.DB) {
        try {
          await env.DB.prepare(`INSERT INTO download_daily (day, file_id, requests) VALUES (?, ?, 1)
            ON CONFLICT(day, file_id) DO UPDATE SET requests = requests + 1`).bind(now().toISOString().slice(0, 10), fileId).run();
        } catch { /* Downloads remain available if aggregate storage is unavailable. */ }
      }
    }
    return new Response(bytes, { headers });
  }

  return {
    async fetch(request, env = {}) {
      let response;
      try {
        const url = new URL(request.url);
        if (newsletterUrl && ['/newsletter', '/newsletter/', '/newsletter/index.html'].includes(url.pathname)) {
          requireMethod(request, ['GET', 'HEAD']);
          response = new Response(null, { status: 302, headers: { Location: newsletterUrl, 'Cache-Control': 'no-store' } });
        } else if (newsletterUrl && url.pathname === '/api/newsletter/subscribe') {
          requireMethod(request, ['POST']);
          // Never forward a submitted email address to a different origin.
          response = json({ ok: false, message: 'Bitte nutze den gemeinsamen NOSRV-Newsletter.', newsletterUrl }, 409);
        } else if (url.pathname === '/api/health') {
          requireMethod(request, ['GET', 'HEAD']);
          response = json({ ok: true, project: config.project });
        } else if (url.pathname === '/api/newsletter/status') {
          requireMethod(request, ['GET', 'HEAD']);
          response = json({ enabled: !newsletterUrl && Boolean(newsletterSettings(env)), consentVersion: NEWSLETTER_CONSENT_VERSION, ...(newsletterUrl ? { newsletterUrl } : {}) });
        } else if (url.pathname === '/api/newsletter/subscribe') {
          requireMethod(request, ['POST']);
          response = await subscribe(request, env);
        } else if (url.pathname === '/api/newsletter/confirm' || url.pathname === '/api/newsletter/unsubscribe') {
          requireMethod(request, ['POST']);
          response = await tokenAction(request, env, url.pathname.endsWith('/confirm') ? 'confirm' : 'unsubscribe');
        } else if (url.pathname === '/api/admin/stats' || url.pathname === '/api/admin/newsletter.csv') {
          requireMethod(request, ['GET']);
          if (!await isAdmin(request, env)) throw new HttpError(401, 'An administrator access code is required.', { 'WWW-Authenticate': 'Bearer' });
          response = url.pathname.endsWith('.csv') ? await newsletterCsv(env) : await stats(url, env);
        } else if (url.pathname.startsWith('/api/')) {
          throw new HttpError(404, 'Endpoint not found.');
        } else {
          requireMethod(request, ['GET', 'HEAD']);
          response = await staticFile(request, url, env);
        }
      } catch (error) {
        const known = error instanceof HttpError;
        response = json({ ok: false, message: known ? error.message : 'The request could not be completed.' }, known ? error.status : 500, known ? error.headers : {});
      }
      const headers = new Headers(response.headers);
      headers.set('X-Content-Type-Options', 'nosniff');
      headers.set('Referrer-Policy', 'no-referrer');
      headers.set('Content-Security-Policy', "object-src 'none'; base-uri 'self'");
      if (request.url.startsWith('https:')) headers.set('Strict-Transport-Security', 'max-age=31536000');
      return new Response(request.method === 'HEAD' ? null : response.body, { status: response.status, statusText: response.statusText, headers });
    },
  };
}

class HttpError extends Error {
  constructor(status, message, headers = {}) { super(message); this.status = status; this.headers = headers; }
}

function json(body, status = 200, headers = {}) {
  return new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'private, no-store', ...headers } });
}

function requireMethod(request, methods) {
  if (!methods.includes(request.method)) throw new HttpError(405, 'Method not allowed.', { Allow: methods.join(', ') });
}

function requireOrigin(request) {
  if (request.headers.get('Origin') !== new URL(request.url).origin || request.headers.get('Sec-Fetch-Site') === 'cross-site') {
    throw new HttpError(403, 'Submit this form from this website.');
  }
}

async function readJson(request) {
  if (!/^application\/json(?:\s*;|$)/i.test(request.headers.get('Content-Type') || '')) throw new HttpError(415, 'Send a JSON form.');
  const declared = request.headers.get('Content-Length');
  if (declared && (!/^\d+$/.test(declared) || Number(declared) > 2048)) throw new HttpError(413, 'This form is too large.');
  if (!request.body) throw new HttpError(400, 'Invalid form.');
  const reader = request.body.getReader();
  const chunks = [];
  let size = 0;
  for (;;) {
    const { value, done } = await reader.read();
    if (done) break;
    size += value.byteLength;
    if (size > 2048) { await reader.cancel(); throw new HttpError(413, 'This form is too large.'); }
    chunks.push(value);
  }
  const bytes = new Uint8Array(size);
  let offset = 0;
  for (const chunk of chunks) { bytes.set(chunk, offset); offset += chunk.byteLength; }
  try {
    const parsed = JSON.parse(new TextDecoder('utf-8', { fatal: true }).decode(bytes));
    if (!parsed || Array.isArray(parsed) || typeof parsed !== 'object') throw new Error();
    return parsed;
  } catch { throw new HttpError(400, 'Invalid form.'); }
}

function normalizeEmail(value) {
  if (typeof value !== 'string') return null;
  const email = value.trim().toLowerCase();
  if (email.length > 254 || !/^[a-z0-9.!#$%&'*+\/=?^_`{|}~-]+@[a-z0-9](?:[a-z0-9-]*[a-z0-9])?(?:\.[a-z0-9](?:[a-z0-9-]*[a-z0-9])?)+$/i.test(email)) return null;
  const [local, domain] = email.split('@');
  if (local.length > 64 || local.startsWith('.') || local.endsWith('.') || local.includes('..') || domain.split('.').some(label => label.length > 63)) return null;
  return email;
}

function safePath(pathname) {
  if (/%(?:2f|5c|00)/i.test(pathname)) throw new HttpError(400, 'Invalid path.');
  let decoded;
  try { decoded = decodeURIComponent(pathname); } catch { throw new HttpError(400, 'Invalid path.'); }
  if (/[\\\u0000-\u001f\u007f]/.test(decoded) || /%(?:2e|2f|5c|00)/i.test(decoded)
    || decoded.split('/').some(part => part === '..' || part === '.')) throw new HttpError(400, 'Invalid path.');
  return decoded;
}

function matchesEtag(header, etag) {
  return Boolean(header && header.split(',').some(value => value.trim() === '*' || value.trim().replace(/^W\//, '') === etag));
}

function byteRange(header, size) {
  const match = /^bytes=(\d*)-(\d*)$/i.exec(header.trim());
  if (!match || (!match[1] && !match[2]) || size === 0) return null;
  if (!match[1]) {
    const suffix = Number(match[2]);
    if (!Number.isSafeInteger(suffix) || suffix < 1) return null;
    return { start: Math.max(0, size - suffix), end: size - 1 };
  }
  const start = Number(match[1]);
  const requestedEnd = match[2] ? Number(match[2]) : size - 1;
  if (!Number.isSafeInteger(start) || !Number.isSafeInteger(requestedEnd) || start >= size || requestedEnd < start) return null;
  return { start, end: Math.min(requestedEnd, size - 1) };
}

function isDate(value) {
  return /^\d{4}-\d{2}-\d{2}$/.test(value) && Number.isFinite(Date.parse(value)) && new Date(value).toISOString().slice(0, 10) === value;
}

function csvCell(value) {
  let cell = String(value);
  if (/^[\s]*[=+\-@\t\r]/.test(cell)) cell = `'${cell}`;
  return `"${cell.replace(/"/g, '""')}"`;
}

function escapeHtml(value) {
  return value.replace(/[&<>"']/g, character => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[character]);
}
