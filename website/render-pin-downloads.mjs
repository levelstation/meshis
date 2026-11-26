import {existsSync} from 'node:fs';
import {readFile,writeFile} from 'node:fs/promises';
import {resolve,join} from 'node:path';
const root=resolve(process.argv[2]||'.'),assets=join(root,existsSync(join(root,'assets/data/download-catalog.json'))?'assets':'docs');
const catalog=JSON.parse(await readFile(join(assets,'data/download-catalog.json'),'utf8'));
const {title}=catalog,rc=catalog.project==='meshis'?'RC7':'RC6';
const esc=value=>String(value).replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;').replaceAll('"','&quot;');
const bytes=value=>new Intl.NumberFormat('de-DE',{maximumFractionDigits:1}).format(value/(value>=1048576?1048576:1024))+(value>=1048576?' MiB':' KiB');
const checksum=r=>`<details class="checksum"><summary>SHA-256 prüfen</summary><div class="checksum-content"><code>${r.sha256}</code><button class="copy-checksum" type="button" data-copy="${r.sha256}" aria-label="SHA-256 für ${esc(r.path.split('/').pop())} kopieren">Prüfsumme kopieren</button></div></details>`;
const meta=r=>`<div class="file-meta"><span class="file-size" data-bytes="${r.bytes}">${bytes(r.bytes)}</span><span class="file-format">${esc(r.path.split('/').pop())}</span></div>${checksum(r)}`;
const attrs=r=>`data-release-id="${r.id}" data-board="${r.board}" data-kind="${r.kind}" data-path="${r.path}"`;
function card(r){
 let heading,copy,action,category,extra='';
 if(r.board==='source'){heading='Gemeinsamer Quellcode';copy='Firmware und Simulatoren mit der neuen PIN-Sperre. Für eigene Builds und zum Nachlesen.';action='Quellcode als ZIP herunterladen';category='WIO UND HELTEC · QUELLCODE';extra=`<a class="text-link" href="https://github.com/levelstation/${catalog.project}" rel="noreferrer">Projekt auf GitHub öffnen</a>`;}
 else if(r.kind==='simulator'){const wio=r.board==='wio';heading=wio?'Wio auf deinem Mac':'Heltec auf deinem Mac';copy=wio?'Joystick-Menüs und vierstellige PIN mit Demodaten ausprobieren.':'PRG-Bedienung und vierstellige PIN mit Demodaten ausprobieren.';action=(wio?'Wio':'Heltec')+'-App für macOS herunterladen';category=(wio?'WIO TRACKER L1 PRO':'HELTEC V4.3')+' · MACOS-APP';extra=`<p class="package-note">Apple Silicon · macOS 14 oder neuer.<br>Ad-hoc signiert, nicht notarisiert.</p><a class="text-link" href="${wio?'/simulator/':'/simulator/heltec/'}">Zuerst im Browser testen</a>`;}
 else if(r.board==='heltec'){const board=r.version.includes('R8')?'R8':'R2';heading='Heltec '+board+' · Anwendungsabbild';copy='Kompilierte PIN-Vorschau für die '+board+'-Variante. Die neue Version ist noch nicht am Gerät geprüft.';action=board+'-Vorschau als BIN herunterladen';category='HELTEC V4.3 · FIRMWARE-VORSCHAU';extra='<p class="package-note">Nur Anwendung · kein vollständiges Erstinstallationspaket. Vor der Verwendung Boardvariante und Partitionierung prüfen.</p><a class="text-link" href="/simulator/heltec/">PRG und PIN im Browser testen</a>';}
 else {heading='Wio-Firmware mit PIN';copy='Vierstelligen Code im Menü „Bildschirmsperre“ erstellen, ändern oder entfernen. PIN-Testbuild für den ursprünglichen Wio Tracker L1 Pro OLED.';action='Wio-PIN-Firmware als DFU ZIP herunterladen';category='WIO TRACKER L1 PRO · FIRMWARE';extra='<p class="hardware-status">Neue PIN-Funktion: <strong>Hardwaretest ausstehend</strong>.</p><a class="text-link" href="/simulator/">PIN zuerst im Browser testen</a>';}
 return `<article class="release-card${r.kind==='firmware'&&r.board==='wio'?' featured':''}" ${attrs(r)}><div class="card-heading"><span class="card-category">${category}</span><span class="version">${esc(r.version)}${r.board==='heltec'?' · Vorschau':''}</span></div><h3>${heading}</h3><p>${copy}</p>${extra}<a class="download-action" href="${r.url}" download>${action}</a>${meta(r)}</article>`;
}
const current=catalog.releases.filter(r=>!(r.board==='wio'&&r.kind==='firmware'&&r.format!=='DFU ZIP')).map(card).join('\n');
const formats=catalog.releases.filter(r=>r.board==='wio'&&r.kind==='firmware'&&r.format!=='DFU ZIP').map(r=>`<article class="format-row" ${attrs(r)}><div class="format-heading"><div><h3>Wio ${rc} · ${r.format}</h3><p>Anwendungsabbild für ein zuvor geprüftes Verfahren.</p></div><a class="download-action" href="${r.url}" download>${r.format} herunterladen</a></div>${meta(r)}</article>`).join('\n');
const advanced=`<details class="advanced-formats" id="advanced-formats" data-board="wio" data-kind="firmware"><summary>Weitere Wio-Firmwareformate<span>BIN und UF2</span></summary><div class="advanced-content"><p class="format-note">Für eigene Werkzeuge und zuvor geprüfte Verfahren. <a href="/FLASHING-GUIDE.md">Pakethinweise</a>.</p>${formats}</div></details>`;
const archiveGroups=new Map();
for(const r of catalog.legacy){const label=r.version||'Bisheriger Stand';if(!archiveGroups.has(label))archiveGroups.set(label,[]);archiveGroups.get(label).push(r);}
const archive=[...archiveGroups].map(([version,rows])=>`<details class="archive-group"><summary>${esc(version)}<span>${rows.length} Dateien</span></summary><ul class="archive-list">${rows.map(r=>`<li class="legacy-row" data-path="${r.path}"><div class="archive-file"><a class="archive-download" href="${r.url}" download>${esc(r.path.split('/').pop())}</a><span class="archive-version">${esc(r.version)} · ${bytes(r.bytes)}</span></div>${checksum(r)}</li>`).join('')}</ul></details>`).join('\n');
const path=join(assets,'downloads/index.html');let html=await readFile(path,'utf8');
const before=html.slice(0,html.indexOf('<section class="archive-section'));
const after=html.slice(html.indexOf('<section class="archive-section'));
let updated=before.replaceAll(catalog.project==='meshis'?'RC6':'RC5',rc).replaceAll('dev3','dev4').replaceAll('2026-11-23','2026-11-26').replaceAll('23. November 2026','26. November 2026').replace('src="downloads.mjs"','src="downloads.mjs?v=pin-20261126"');
updated=updated.replace(/<div class="current-releases" id="current-releases">[\s\S]*?<details class="advanced-formats" id="advanced-formats"[\s\S]*?<article class="empty-state" id="empty-state"[\s\S]*?<p class="metadata-note">/,`<div class="current-releases" id="current-releases">${current}</div>\n${advanced}\n<article class="empty-state" id="empty-state" hidden><h3>Keine Datei für diese Auswahl.</h3><p>Wähle ein Board und einen anderen Dateityp.</p></article>\n<p class="metadata-note">`);
updated=updated.replace(/<p class="metadata-note">[\s\S]*?<\/p>/,'<p class="metadata-note">Größen und SHA-256 stammen von den bereitgestellten Dateien. <a href="/downloads/pin/SHA256SUMS">Aktuelle Prüfsummen</a> · <a href="/SCREEN-PIN.md">PIN-Bedienung</a></p>');
updated=updated.replace('<a href="/TEST-REPORT.md">Testbericht</a>','<a href="/SCREEN-PIN.md">PIN-Sperre</a>');
updated+=after.replace(/<div id="archive-groups">[\s\S]*?<div id="additional-archive"><\/div>/,`<div id="archive-groups">${archive}</div><div id="additional-archive"></div>`);
if(!updated.includes('data-release-id="heltec-dev4-r2-bin"')||!updated.includes('source-pin'))throw Error('PIN download rendering failed');
await writeFile(path,updated);
console.log(JSON.stringify({result:'PASS',project:catalog.project,current:catalog.releases.length,legacy:catalog.legacy.length}));
