const $=id=>document.getElementById(id);let generation=0;let access='',stats=null,catalog={releases:[],legacy:[]};
const number=value=>new Intl.NumberFormat('de-DE').format(Number(value)||0);
function filename(id){const item=[...catalog.releases,...catalog.legacy].find(item=>item.id===id);return item?`${item.title} · ${item.version} · ${item.format}`:id;}
function cell(text){const td=document.createElement('td');td.textContent=String(text);return td;}
function renderRows(node,rows){node.replaceChildren();for(const row of rows){const tr=document.createElement('tr');for(const value of row)tr.append(cell(value));node.append(tr);}if(!rows.length){const tr=document.createElement('tr');const td=cell('Noch keine Downloadaufrufe in diesem Zeitraum.');td.colSpan=3;tr.append(td);node.append(tr);}}
async function load(){if(!access)return;const requestGeneration=++generation;$('stats-status').textContent='Statistik wird geladen …';try{const response=await fetch(`/api/admin/stats?days=${encodeURIComponent($('stats-days').value)}`,{headers:{Authorization:`Bearer ${access}`},cache:'no-store'});if(response.status===401)throw Error('Der Zugangscode ist ungültig.');if(!response.ok)throw Error('Die Statistik ist vorübergehend nicht verfügbar.');const data=await response.json();if(requestGeneration!==generation||!access)return;stats=data;$('login-card').hidden=true;$('stats-dashboard').hidden=false;$('period-total').textContent=number(stats.downloads.total);$('all-total').textContent=number(stats.downloads.allTimeTotal);$('subscriber-total').textContent=number(stats.newsletter.confirmed);$('stats-dates').textContent=`${stats.from} bis ${stats.to} · UTC · vollständige Downloadaufrufe`;renderRows($('file-counts'),stats.downloads.byFile.map(row=>[filename(row.file_id),number(row.requests)]));renderRows($('day-counts'),stats.downloads.byDay.map(row=>[row.day,filename(row.file_id),number(row.requests)]));$('stats-status').textContent='Aktuell. Diese Zahlen sind nur mit Verwaltungszugang abrufbar.';}catch(error){if(requestGeneration===generation&&access)$('stats-status').textContent=error.message;}}
function save(blob,name){const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function csvCell(text){let value=String(text);if(/^[\s]*[=+\-@]/.test(value))value="'"+value;return `"${value.replaceAll('"','""')}"`;}
$('stats-login').addEventListener('submit',event=>{event.preventDefault();access=$('access-code').value.trim();$('access-code').value='';load();});$('stats-days').addEventListener('change',load);$('refresh-stats').addEventListener('click',load);$('stats-logout').addEventListener('click',()=>{++generation;access='';stats=null;$('stats-dashboard').hidden=true;$('login-card').hidden=false;for(const id of ['file-counts','day-counts'])$(id).replaceChildren();$('stats-status').textContent='Abgemeldet. Der Zugangscode wurde aus diesem Tab entfernt.';});
$('export-downloads').addEventListener('click',()=>{if(!stats)return;const rows=[['Datei','Von (UTC)','Bis (UTC)','Downloadaufrufe'],...stats.downloads.byFile.map(row=>[filename(row.file_id),stats.from,stats.to,row.requests])];save(new Blob(['\uFEFF'+rows.map(row=>row.map(csvCell).join(';')).join('\r\n')],{type:'text/csv;charset=utf-8'}),`${stats.project}-downloads-${stats.from}-${stats.to}.csv`);});
async function exportSubscribers(){
 if(!access)return;const exportGeneration=generation;
 try {
  const response=await fetch('/api/admin/newsletter.csv',{headers:{Authorization:`Bearer ${access}`},cache:'no-store'});
  if(exportGeneration!==generation||!access){await response.body?.cancel();return;}
  if(!response.ok)throw Error('Der Newsletter-Export ist gerade nicht verfügbar.');
  const blob=await response.blob();if(exportGeneration!==generation||!access)return;
  save(blob,`${document.body.dataset.project}-newsletter.csv`);
 }catch(error){if(exportGeneration===generation&&access)$('stats-status').textContent=error.message;}
}
$('export-subscribers').addEventListener('click',exportSubscribers);
fetch('/data/download-catalog.json',{credentials:'omit'}).then(r=>r.ok?r.json():null).then(data=>{if(data)catalog=data;}).catch(()=>{});
fetch('/api/newsletter/status',{credentials:'omit',cache:'no-store'}).then(r=>r.ok?r.json():null).then(data=>{$('export-subscribers').hidden=!data?.enabled;}).catch(()=>{});
