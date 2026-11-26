/** Download filters also work with the embedded metadata when the catalog is unavailable. */
export function releaseMatches(release, board, kind) {
  return release.kind === kind && (kind === 'source' || release.board === board);
}

export function formatBytes(bytes) {
  if (!Number.isFinite(bytes) || bytes < 0) return '';
  const unit = bytes >= 1024 ** 2 ? 'MiB' : bytes >= 1024 ? 'KiB' : 'Bytes';
  const divisor = unit === 'MiB' ? 1024 ** 2 : unit === 'KiB' ? 1024 : 1;
  return `${new Intl.NumberFormat('de-DE', { maximumFractionDigits: 1 }).format(bytes / divisor)} ${unit}`;
}

export function validRelease(release) {
  return Boolean(release && /^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(release.id)
    && ['wio', 'heltec', 'source'].includes(release.board)
    && ['firmware', 'simulator', 'source'].includes(release.kind)
    && Number.isSafeInteger(release.bytes) && release.bytes > 0
    && /^[a-f0-9]{64}$/i.test(release.sha256)
    && /^\/downloads\/[a-zA-Z0-9_./-]+$/.test(release.path)
    && !release.path.split('/').includes('..')
    && release.url === `/get/${release.id}`);
}

export function selectionFromSearch(search) {
  const params = new URLSearchParams(search);
  return {
    board: params.get('board') === 'heltec' ? 'heltec' : 'wio',
    kind: ['firmware', 'simulator', 'source'].includes(params.get('kind')) ? params.get('kind') : 'firmware',
  };
}

function enhanceDownloads() {
  const form = document.querySelector('#download-filters');
  if (!form) return;
  const cards = [...document.querySelectorAll('#current-releases [data-release-id]')];
  const advanced = document.querySelector('#advanced-formats');
  const status = document.querySelector('#selection-status');
  const empty = document.querySelector('#empty-state');
  const initial = selectionFromSearch(location.search);
  for (const [name, value] of Object.entries(initial)) form.elements.namedItem(name).value = value;

  function applyFilters(updateAddress = true) {
    const board = form.elements.namedItem('board').value;
    const kind = form.elements.namedItem('kind').value;
    let visible = 0;
    cards.forEach(card => {
      card.hidden = !releaseMatches(card.dataset, board, kind);
      if (!card.hidden) visible += 1;
    });
    advanced.hidden = !releaseMatches(advanced.dataset, board, kind);
    if (!advanced.hidden) visible += advanced.querySelectorAll('[data-release-id]').length;
    empty.hidden = visible > 0;
    document.querySelector('#current-releases').classList.add('filtered');
    const boardName = board === 'wio' ? 'Wio Tracker L1 Pro' : 'Heltec V4.3';
    const kindName = { firmware: 'Firmware', simulator: 'macOS-App', source: 'Quellcode' }[kind];
    status.textContent = kind === 'source' ? 'Gemeinsamer Quellcode für Wio und Heltec · Stand 26.11.2026'
      : `${kindName} für ${boardName} · ${visible === 0 ? 'Vorschau ohne Flash-Paket' : visible === 1 ? '1 Datei' : `${visible} Dateien`} · Stand 26.11.2026`;
    if (updateAddress) {
      const address = new URL(location.href);
      address.searchParams.set('board', board);
      address.searchParams.set('kind', kind);
      history.replaceState(null, '', address);
    }
  }

  form.hidden = false;
  form.addEventListener('change', () => applyFilters());
  form.addEventListener('submit', event => event.preventDefault());
  document.querySelectorAll('[data-select-kind]').forEach(button => {
    button.addEventListener('click', () => {
      form.elements.namedItem('kind').value = button.dataset.selectKind;
      applyFilters();
      document.querySelector('#downloads-title').scrollIntoView({ block: 'start' });
      const selected = form.querySelector(`input[name="kind"][value="${button.dataset.selectKind}"]`);
      selected.focus({ preventScroll: true });
    });
  });
  applyFilters(false);

  document.addEventListener('click', async event => {
    const button = event.target.closest?.('[data-copy]');
    if (!button) return;
    const announcement = document.querySelector('#copy-status');
    try {
      await navigator.clipboard.writeText(button.dataset.copy);
      button.textContent = 'Kopiert';
      announcement.textContent = 'SHA-256-Prüfsumme kopiert.';
      setTimeout(() => { button.textContent = 'Prüfsumme kopieren'; }, 2500);
    } catch {
      announcement.textContent = 'Bitte die angezeigte SHA-256-Prüfsumme markieren und kopieren.';
      const code = button.closest('.checksum-content').querySelector('code');
      const range = document.createRange();
      range.selectNodeContents(code);
      const selection = window.getSelection();
      selection.removeAllRanges();
      selection.addRange(range);
    }
  });

  // This request contains no personal identifiers. All file links work without it.
  fetch('/data/download-catalog.json', { credentials: 'omit' })
    .then(response => response.ok ? response.json() : Promise.reject())
    .then(catalog => {
      if (catalog.project !== document.body.dataset.project || !Array.isArray(catalog.releases)) return;
      for (const release of catalog.releases.filter(validRelease)) {
        const element = document.querySelector(`[data-release-id="${release.id}"]`);
        if (!element || element.dataset.path !== release.path) continue;
        const size = element.querySelector('.file-size');
        size.textContent = formatBytes(release.bytes);
        size.dataset.bytes = String(release.bytes);
        element.querySelector('.download-action').href = release.url;
        element.querySelector('.checksum-content code').textContent = release.sha256;
        element.querySelector('[data-copy]').dataset.copy = release.sha256;
      }
      addCatalogArchive(Array.isArray(catalog.legacy) ? catalog.legacy : []);
    })
    .catch(() => { /* Embedded, verified metadata remains available. */ });
}

function addCatalogArchive(releases) {
  const knownPaths = new Set([...document.querySelectorAll('[data-path]')].map(element => element.dataset.path));
  const additional = releases.filter(validRelease).filter(release => !knownPaths.has(release.path));
  if (!additional.length) return;
  const group = document.createElement('details');
  group.className = 'archive-group';
  const heading = document.createElement('summary');
  heading.textContent = 'Weitere bisherige Versionen';
  const list = document.createElement('ul');
  list.className = 'archive-list';
  for (const release of additional) {
    const row = document.createElement('li');
    row.className = 'legacy-row';
    row.dataset.path = release.path;
    const file = document.createElement('div');
    file.className = 'archive-file';
    const link = document.createElement('a');
    link.className = 'archive-download';
    link.href = release.url;
    link.download = '';
    link.textContent = release.title || release.path.split('/').pop();
    const version = document.createElement('span');
    version.className = 'archive-version';
    version.textContent = `${release.version || 'Bisheriger Stand'} · ${formatBytes(release.bytes)}`;
    const checksum = document.createElement('details');
    checksum.className = 'checksum';
    const summary = document.createElement('summary');
    summary.textContent = 'SHA-256';
    const checksumContent = document.createElement('div');
    checksumContent.className = 'checksum-content';
    const code = document.createElement('code');
    code.textContent = release.sha256;
    const copy = document.createElement('button');
    copy.type = 'button';
    copy.className = 'copy-checksum';
    copy.dataset.copy = release.sha256;
    copy.textContent = 'Prüfsumme kopieren';
    copy.setAttribute('aria-label', `SHA-256 für ${link.textContent} kopieren`);
    checksumContent.append(code, copy);
    checksum.append(summary, checksumContent);
    file.append(link, version);
    row.append(file, checksum);
    list.append(row);
    knownPaths.add(release.path);
  }
  group.append(heading, list);
  document.querySelector('#additional-archive').append(group);
}

if (typeof document !== 'undefined') enhanceDownloads();
