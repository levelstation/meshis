import test from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { DatabaseSync } from 'node:sqlite';
import { createSiteWorker, NEWSLETTER_CONSENT_VERSION } from './worker.mjs';

const ORIGIN = 'https://example.test';
const SECRET = 'test-only-administrator-code';
const DIGEST = value => createHash('sha256').update(value).digest('hex');
const NOW = '2026-11-23T12:00:00.000Z';
const FILE = Uint8Array.from([0, 255, 17, 34, 51, 128, 254, 10]);

// The adapter executes the actual Worker SQL against SQLite, without a network.
class D1 {
  constructor() {
    this.sqlite = new DatabaseSync(':memory:');
    this.sqlite.exec(`CREATE TABLE download_daily (day TEXT NOT NULL, file_id TEXT NOT NULL, requests INTEGER NOT NULL, PRIMARY KEY(day, file_id));
      CREATE TABLE newsletter_subscribers (email TEXT PRIMARY KEY, status TEXT NOT NULL, consent_version TEXT NOT NULL,
        consented_at TEXT NOT NULL, confirmed_at TEXT, confirmation_hash TEXT, confirmation_expires_at TEXT,
        unsubscribe_hash TEXT NOT NULL, mail_sent_at TEXT, updated_at TEXT NOT NULL);
      CREATE TABLE newsletter_send_budget (minute TEXT PRIMARY KEY, requests INTEGER NOT NULL);`);
    this.statements = [];
  }
  prepare(sql) {
    const db = this;
    return {
      values: [],
      bind(...values) { this.values = values; return this; },
      async first() { db.statements.push({ sql, values: this.values }); return db.sqlite.prepare(sql).get(...this.values) || null; },
      async all() { db.statements.push({ sql, values: this.values }); return { results: db.sqlite.prepare(sql).all(...this.values) }; },
      execute() { db.statements.push({ sql, values: this.values }); return { meta: db.sqlite.prepare(sql).run(...this.values) }; },
      async run() { return this.execute(); },
    };
  }
  async batch(statements) {
    this.sqlite.exec('BEGIN');
    try { const results = statements.map(statement => statement.execute()); this.sqlite.exec('COMMIT'); return results; }
    catch (error) { this.sqlite.exec('ROLLBACK'); throw error; }
  }
  rows(table) { return this.sqlite.prepare(`SELECT * FROM ${table}`).all().map(row => ({ ...row })); }
}

function asset(value, type = 'text/html; charset=utf-8') {
  const bytes = typeof value === 'string' ? Buffer.from(value) : Buffer.from(value);
  return { data: bytes.toString('base64'), bytes: bytes.byteLength, sha256: DIGEST(bytes), type };
}

function setup(options = {}) {
  const db = new D1();
  const messages = [];
  let time = NOW;
  const config = {
    project: 'example', title: 'Example <site>', canonicalOrigin: ORIGIN,
    assets: { '/index.html': asset('<html>Home</html>'), '/newsletter/index.html': asset('<html>Newsletter</html>'),
      '/hello%world.txt': asset('encoded name', 'text/plain'), '/downloads/example.uf2': asset(FILE, 'application/octet-stream'),
      '/downloads/legacy.bin': asset(FILE, 'application/octet-stream'), '/empty.txt': asset('', 'text/plain') },
    catalog: { releases: [{ id: 'release-v1', path: '/downloads/example.uf2' }], legacy: [{ id: 'legacy', path: '/downloads/legacy.bin' }] },
  };
  const worker = createSiteWorker(config, { now: () => time,
    fetch: async (url, init) => { messages.push({ url, ...init, parsed: JSON.parse(init.body) }); return new Response('{}', { status: options.mailFailure ? 503 : 202 }); } });
  const env = { DB: db, ADMIN_ACCESS_HASH: DIGEST(SECRET), ...options.env };
  const call = (path, init = {}, environment = env) => worker.fetch(new Request(`${ORIGIN}${path}`, init), environment);
  const post = (path, body, init = {}, environment = env) => call(path, { method: 'POST', ...init,
    headers: { Origin: ORIGIN, 'Content-Type': 'application/json', ...init.headers }, body: JSON.stringify(body) }, environment);
  const enabled = () => Object.assign(env, { NEWSLETTER_ENABLED: 'true', NEWSLETTER_PROVIDER: 'resend', NEWSLETTER_API_KEY: 'test-only-key', NEWSLETTER_FROM_EMAIL: 'updates@example.test' });
  const subscribe = (email = 'person@example.test', extra = {}) => post('/api/newsletter/subscribe', {
    email, consent: true, consentVersion: NEWSLETTER_CONSENT_VERSION, company: '', ...extra,
  });
  return { db, env, config, messages, call, post, enabled, subscribe, advance: milliseconds => { time = new Date(Date.parse(time) + milliseconds).toISOString(); } };
}

function mailTokens(message) {
  const text = message.parsed.text || message.parsed.textContent;
  return { confirm: /#confirm=([A-Za-z0-9_-]+)/.exec(text)[1], unsubscribe: /#unsubscribe=([A-Za-z0-9_-]+)/.exec(text)[1], text };
}

test('all static and registered bytes survive, directory index, security and ETags', async () => {
  const s = setup();
  for (const path of ['/', '/index.html', '/newsletter/']) {
    const response = await s.call(path);
    assert.equal(response.status, 200);
    assert.match(await response.text(), /<html>/);
    assert.equal(response.headers.get('X-Content-Type-Options'), 'nosniff');
    assert.equal(response.headers.get('Referrer-Policy'), 'no-referrer');
  }
  const directory = await s.call('/newsletter?source=test');
  assert.equal(directory.status, 308);
  assert.equal(directory.headers.get('Location'), '/newsletter/?source=test');
  assert.equal(await (await s.call('/hello%25world.txt')).text(), 'encoded name');
  const response = await s.call('/downloads/example.uf2');
  assert.deepEqual(new Uint8Array(await response.arrayBuffer()), FILE);
  assert.equal(response.headers.get('ETag'), `"${DIGEST(FILE)}"`);
  assert.match(response.headers.get('Content-Disposition'), /example.uf2/);
  assert.match(response.headers.get('Cache-Control'), /no-store/);
  const again = await s.call('/downloads/example.uf2', { headers: { 'If-None-Match': `W/"${DIGEST(FILE)}"` } });
  assert.equal(again.status, 304);
  assert.equal(await again.text(), '');
  assert.equal(s.db.rows('download_daily')[0].requests, 1);
  const head = await s.call('/downloads/example.uf2', { method: 'HEAD' });
  assert.equal(head.status, 200);
  assert.equal(await head.text(), '');
  assert.equal(head.headers.get('Content-Length'), String(FILE.length));
  assert.match(head.headers.get('Cache-Control'), /no-store/);
  assert.equal(s.db.rows('download_daily')[0].requests, 1);
});

test('GET /get/:id serves the file without a redirect and increments aggregate atomically', async () => {
  const s = setup();
  const responses = await Promise.all(Array.from({ length: 25 }, () => s.call('/get/release-v1')));
  for (const response of responses) {
    assert.equal(response.status, 200);
    assert.equal(response.headers.has('Location'), false);
    assert.deepEqual(new Uint8Array(await response.arrayBuffer()), FILE);
    assert.match(response.headers.get('Cache-Control'), /no-store/);
  }
  await s.call('/downloads/example.uf2');
  await s.call('/get/legacy');
  assert.deepEqual(s.db.rows('download_daily'), [
    { day: '2026-11-23', file_id: 'release-v1', requests: 26 },
    { day: '2026-11-23', file_id: 'legacy', requests: 1 },
  ]);
  assert.ok(s.db.statements.filter(row => row.sql.includes('INSERT INTO download_daily')).every(row => /ON CONFLICT/.test(row.sql)));
  assert.equal((await s.call('/get/unknown')).status, 404);
  assert.equal((await s.call('/get/release-v1', { method: 'POST' })).status, 405);
});

test('ranges, suffixes, HEAD, invalid ranges and prefetch never increment', async () => {
  const s = setup();
  for (const [range, expected, contentRange] of [
    ['bytes=1-3', FILE.slice(1, 4), 'bytes 1-3/8'], ['bytes=-3', FILE.slice(-3), 'bytes 5-7/8'],
    ['bytes=6-', FILE.slice(6), 'bytes 6-7/8'], ['bytes=0-999', FILE, 'bytes 0-7/8'],
  ]) {
    const response = await s.call('/get/release-v1', { headers: { Range: range } });
    assert.equal(response.status, 206);
    assert.equal(response.headers.get('Content-Range'), contentRange);
    assert.deepEqual(new Uint8Array(await response.arrayBuffer()), expected);
    assert.match(response.headers.get('Cache-Control'), /no-store/);
  }
  for (const range of ['bytes=8-', 'bytes=-0', 'bytes=4-1', 'bytes=0-1,4-6', 'other=0-1', 'bytes=-', 'bytes=999999999999999999-']) {
    const response = await s.call('/get/release-v1', { headers: { Range: range } });
    assert.equal(response.status, 416, range);
    assert.equal(response.headers.get('Content-Range'), 'bytes */8');
  }
  assert.equal((await s.call('/empty.txt', { headers: { Range: 'bytes=0-' } })).status, 416);
  assert.equal((await s.call('/get/release-v1', { method: 'HEAD', headers: { Range: 'bytes=0-1' } })).status, 200);
  for (const headers of [{ Purpose: 'prefetch' }, { 'Sec-Purpose': 'prefetch;prerender' }, { Range: 'bytes=0-1', 'If-Range': '"different"' }]) {
    assert.equal((await s.call('/get/release-v1', { headers })).status, 200);
  }
  assert.deepEqual(s.db.rows('download_daily'), []);
});

test('downloads fail open without D1 and no private access code is required', async () => {
  const s = setup();
  for (const environment of [{}, { DB: { prepare() { throw new Error('private infrastructure failure'); } } }]) {
    const response = await s.call('/get/release-v1', {}, environment);
    assert.equal(response.status, 200);
    assert.deepEqual(new Uint8Array(await response.arrayBuffer()), FILE);
  }
});

test('only an authenticated diagnostic header can exclude a successful download', async () => {
  const s = setup();
  await s.call('/get/release-v1', { headers: { 'X-Download-Verification': '1' } });
  await s.call('/get/release-v1', { headers: { 'X-Download-Verification': '1', Authorization: 'Bearer wrong' } });
  await s.call('/get/release-v1', { headers: { 'X-Download-Verification': '1', Authorization: `Bearer ${SECRET}` } });
  await s.call('/get/release-v1', { headers: { Authorization: `Bearer ${SECRET}` } });
  assert.equal(s.db.rows('download_daily')[0].requests, 3);
});

test('unknown paths and encoded traversal never expose implementation or private state', async () => {
  const s = setup();
  for (const path of ['/.openai/hosting.json', '/worker.mjs', '/__proto__', '/not-found']) assert.equal((await s.call(path)).status, 404);
  for (const path of ['/downloads%2Fexample.uf2', '/%5csecret', '/%00', '/%252e%252e/secret', '/%GG']) assert.equal((await s.call(path)).status, 400);
  const health = await (await s.call('/api/health')).json();
  assert.deepEqual(health, { ok: true, project: 'example' });
  assert.equal((await s.call('/api/admin/stats', { headers: { 'oai-authenticated-user-email': 'owner@example.test' } })).status, 401);
});

test('admin aggregate API authorizes Bearer hash, scopes dates, and omits all subscriber addresses', async () => {
  const s = setup();
  await s.call('/get/release-v1');
  s.db.sqlite.prepare('INSERT INTO download_daily VALUES (?, ?, ?)').run('2026-01-01', 'legacy', 9);
  s.db.sqlite.prepare('INSERT INTO download_daily VALUES (?, ?, ?)').run('2026-11-22', 'legacy', 4);
  s.enabled(); await s.subscribe();
  for (const headers of [{}, { Authorization: 'Bearer wrong' }, { 'OAI-Sites-Authorization': `Bearer ${SECRET}` }]) {
    const response = await s.call('/api/admin/stats', { headers });
    assert.equal(response.status, 401);
    assert.equal(response.headers.get('WWW-Authenticate'), 'Bearer');
    assert.match(response.headers.get('Cache-Control'), /no-store/);
  }
  const headers = { Authorization: `Bearer ${SECRET}` };
  const response = await s.call('/api/admin/stats?days=1', { headers });
  assert.equal(response.status, 200);
  const raw = await response.text();
  assert.equal(raw.includes('person@example.test'), false);
  assert.equal(raw.includes(SECRET), false);
  const stats = JSON.parse(raw);
  assert.equal(stats.downloads.total, 1);
  assert.equal(stats.downloads.allTimeTotal, 14);
  assert.deepEqual(stats.newsletter, { pending: 1, confirmed: 0, unsubscribed: 0 });
  assert.equal((await s.call('/api/admin/stats?from=2025-01-01&to=2030-01-01', { headers })).status, 200);
  assert.equal((await s.call('/api/admin/stats?to=2030-01-01', { headers })).status, 200);
  for (const query of ['days=0', 'days=91', 'days=1.5', 'from=2026-02-30', 'from=2027-01-01', 'to=invalid']) {
    assert.equal((await s.call(`/api/admin/stats?${query}`, { headers })).status, 400, query);
  }
});

test('newsletter is disabled without complete provider configuration and never stores addresses', async () => {
  const s = setup();
  assert.deepEqual(await (await s.call('/api/newsletter/status')).json(), { enabled: false, consentVersion: NEWSLETTER_CONSENT_VERSION });
  for (const settings of [{}, { NEWSLETTER_PROVIDER: 'resend' }, { NEWSLETTER_PROVIDER: 'unknown', NEWSLETTER_API_KEY: 'key', NEWSLETTER_FROM_EMAIL: 'from@example.test' }]) {
    Object.assign(s.env, settings);
    assert.equal((await s.subscribe()).status, 503);
  }
  assert.deepEqual(s.db.rows('newsletter_subscribers'), []);
  assert.deepEqual(s.db.rows('newsletter_send_budget'), []);
  assert.equal(s.messages.length, 0);
});

test('newsletter requires explicit consent, exact version, valid small JSON and same-origin POST', async () => {
  const s = setup(); s.enabled();
  for (const extra of [{ consent: false }, { consent: 'true' }, { consentVersion: 'old' }, { email: 'bad@' }, { email: `x\r\nBcc:other@example.test` }, { email: 'a'.repeat(65) + '@example.test' }]) {
    assert.equal((await s.subscribe('person@example.test', extra)).status, 400);
  }
  for (const headers of [{ Origin: 'https://attacker.test' }, { Origin: '' }, { Origin: ORIGIN, 'Sec-Fetch-Site': 'cross-site' }]) {
    assert.equal((await s.post('/api/newsletter/subscribe', {}, { headers })).status, 403);
  }
  assert.equal((await s.call('/api/newsletter/subscribe')).status, 405);
  assert.equal((await s.post('/api/newsletter/subscribe', {}, { headers: { 'Content-Type': 'text/plain' } })).status, 415);
  assert.equal((await s.post('/api/newsletter/subscribe', { padding: 'x'.repeat(2049) })).status, 413);
  assert.equal((await s.call('/api/newsletter/subscribe', { method: 'POST', headers: { Origin: ORIGIN, 'Content-Type': 'application/json' }, body: '{' })).status, 400);
  assert.equal((await s.subscribe('person@example.test', { company: 'bot company' })).status, 202);
  assert.deepEqual(s.db.rows('newsletter_subscribers'), []);
  assert.equal(s.messages.length, 0);
});

test('confirmation and unsubscribe use fragment tokens, hashed storage, explicit POST and expiry', async () => {
  const s = setup(); s.enabled();
  const response = await s.subscribe('Person@Example.test');
  assert.equal(response.status, 202);
  assert.equal((await response.text()).includes('Person'), false);
  assert.equal(s.messages.length, 1);
  assert.equal(s.messages[0].url, 'https://api.resend.com/emails');
  assert.equal(s.messages[0].headers.Authorization, 'Bearer test-only-key');
  assert.match(s.messages[0].parsed.html, /Example &lt;site&gt;/);
  const tokens = mailTokens(s.messages[0]);
  assert.equal(tokens.confirm.length, 43);
  assert.equal(tokens.unsubscribe.length, 43);
  assert.equal(tokens.text.includes('?'), false);
  let row = s.db.rows('newsletter_subscribers')[0];
  assert.equal(row.email, 'person@example.test');
  assert.equal(row.status, 'pending');
  assert.equal(row.confirmation_hash, DIGEST(tokens.confirm));
  assert.equal(row.unsubscribe_hash, DIGEST(tokens.unsubscribe));
  assert.equal(row.confirmation_expires_at, '2026-11-24T12:00:00.000Z');
  assert.equal(row.mail_sent_at, NOW);
  assert.equal(JSON.stringify(s.db.rows('newsletter_subscribers')).includes(tokens.confirm), false);
  await s.call(`/newsletter/#confirm=${tokens.confirm}`);
  assert.equal((await s.call(`/api/newsletter/confirm?token=${tokens.confirm}`)).status, 405);
  assert.equal(s.db.rows('newsletter_subscribers')[0].status, 'pending');
  assert.equal((await s.post('/api/newsletter/confirm', { token: tokens.confirm })).status, 200);
  row = s.db.rows('newsletter_subscribers')[0];
  assert.equal(row.status, 'confirmed');
  assert.equal(row.confirmation_hash, null);
  assert.equal((await s.post('/api/newsletter/confirm', { token: tokens.confirm })).status, 400);
  assert.equal((await s.post('/api/newsletter/unsubscribe', { token: tokens.unsubscribe })).status, 200);
  assert.equal((await s.post('/api/newsletter/unsubscribe', { token: tokens.unsubscribe })).status, 200);
  assert.equal(s.db.rows('newsletter_subscribers')[0].status, 'unsubscribed');
  await s.subscribe('expired@example.test');
  const expired = mailTokens(s.messages[1]);
  s.advance(86_400_001);
  assert.equal((await s.post('/api/newsletter/confirm', { token: expired.confirm })).status, 400);
});

test('cooldown, atomic resend, confirmed duplicates, global budget and pending retention', async () => {
  const s = setup(); s.enabled();
  await s.subscribe();
  const oldTokens = mailTokens(s.messages[0]);
  await s.subscribe();
  assert.equal(s.messages.length, 1);
  s.advance(15 * 60_000);
  const concurrent = await Promise.all([s.subscribe(), s.subscribe()]);
  assert.ok(concurrent.every(response => response.status === 202));
  assert.equal(s.messages.length, 2);
  assert.equal((await s.post('/api/newsletter/confirm', { token: oldTokens.confirm })).status, 400);
  await s.post('/api/newsletter/confirm', { token: mailTokens(s.messages[1]).confirm });
  s.advance(15 * 60_000);
  await s.subscribe();
  assert.equal(s.messages.length, 2);
  for (let index = 0; index < 5; index++) assert.equal((await s.subscribe(`other${index}@example.test`)).status, 202);
  assert.equal((await s.subscribe('too-many@example.test')).status, 429);
  assert.equal(s.db.rows('newsletter_subscribers').some(row => row.email === 'too-many@example.test'), false);
  s.advance(8 * 86_400_000);
  await s.subscribe('new@example.test');
  assert.equal(s.db.rows('newsletter_subscribers').filter(row => row.status === 'pending').length, 1);
  assert.equal(s.db.rows('newsletter_subscribers').some(row => row.status === 'confirmed'), true);
});

test('provider failure never confirms, redacts provider errors and retains cooldown', async () => {
  const s = setup({ mailFailure: true }); s.enabled();
  const response = await s.subscribe();
  assert.equal(response.status, 503);
  const body = await response.text();
  assert.equal(body.includes('person@example.test'), false);
  assert.equal(body.includes('test-only-key'), false);
  assert.equal(s.db.rows('newsletter_subscribers')[0].status, 'pending');
  assert.equal(s.db.rows('newsletter_subscribers')[0].mail_sent_at, null);
  await s.subscribe();
  assert.equal(s.messages.length, 1);
});

test('concurrent new subscribers cannot exceed the atomic five-per-minute sending budget', async () => {
  const s = setup(); s.enabled();
  const responses = await Promise.all(Array.from({ length: 12 }, (_, index) => s.subscribe(`concurrent${index}@example.test`)));
  assert.equal(responses.filter(response => response.status === 202).length, 5);
  assert.equal(responses.filter(response => response.status === 429).length, 7);
  assert.equal(s.messages.length, 5);
  assert.equal(s.db.rows('newsletter_send_budget')[0].requests, 5);
  assert.equal(s.db.rows('newsletter_subscribers').length, 5);
});

test('Brevo transport is fixed server-side and sends only a confirmation', async () => {
  const s = setup(); s.enabled(); s.env.NEWSLETTER_PROVIDER = 'brevo';
  assert.equal((await s.subscribe()).status, 202);
  assert.equal(s.messages[0].url, 'https://api.brevo.com/v3/smtp/email');
  assert.equal(s.messages[0].headers['api-key'], 'test-only-key');
  assert.equal(s.messages[0].parsed.to[0].email, 'person@example.test');
  assert.match(s.messages[0].parsed.textContent, /confirm/i);
});

test('protected CSV exports only confirmed contacts and neutralizes formulas', async () => {
  const s = setup(); s.enabled();
  await s.subscribe('=formula@example.test');
  await s.post('/api/newsletter/confirm', { token: mailTokens(s.messages[0]).confirm });
  await s.subscribe('pending@example.test');
  assert.equal((await s.call('/api/admin/newsletter.csv')).status, 401);
  const response = await s.call('/api/admin/newsletter.csv', { headers: { Authorization: `Bearer ${SECRET}` } });
  assert.equal(response.status, 200);
  assert.match(response.headers.get('Content-Type'), /text\/csv/);
  assert.match(response.headers.get('Cache-Control'), /no-store/);
  const csv = await response.text();
  assert.match(csv, /"'=formula@example.test"/);
  assert.equal(csv.includes('pending@example.test'), false);
  assert.equal(csv.includes('confirmation_hash'), false);
  assert.equal(csv.includes('unsubscribe_hash'), false);
});

test('SQL writes contain no visitor metadata or browser identity', async () => {
  const s = setup();
  await s.call('/get/release-v1', { headers: { 'CF-Connecting-IP': '192.0.2.1', 'User-Agent': 'sensitive-agent', Cookie: 'visitor=secret', 'oai-authenticated-user-id': 'private-user' } });
  const persisted = JSON.stringify(s.db.statements);
  for (const secret of ['192.0.2.1', 'sensitive-agent', 'visitor=secret', 'private-user']) assert.equal(persisted.includes(secret), false);
});

test('newsletter requires explicit operator activation even with complete credentials',async()=>{const s=setup();s.enabled();delete s.env.NEWSLETTER_ENABLED;const status=await s.call('/api/newsletter/status');assert.equal((await status.json()).enabled,false);assert.equal((await s.subscribe()).status,503);assert.equal(s.db.rows('newsletter_subscribers').length,0);assert.equal(s.messages.length,0);});
