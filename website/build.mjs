import {readFile,readdir,mkdir,writeFile,rename,stat} from 'node:fs/promises';
import {resolve,join,relative,dirname} from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const here=dirname(fileURLToPath(import.meta.url));
const cwd=resolve(process.cwd());const assetsDir=resolve(process.argv[2]||join(cwd,'assets'));const outDir=resolve(process.argv[3]||join(cwd,'dist'));
const manifest=JSON.parse(await readFile(join(cwd,'.openai/hosting.json'),'utf8'));
if(manifest.static||manifest.d1!=='DB'||!manifest.project_id)throw Error('Expected retained Sites Worker manifest with DB binding');
const project=JSON.parse(await readFile(join(cwd,'website/project.json'),'utf8'));
const catalog=JSON.parse(await readFile(join(assetsDir,'data/download-catalog.json'),'utf8'));
if(catalog.project!==project.project)throw Error('Catalogue/project mismatch');
const types={'.html':'text/html; charset=utf-8','.css':'text/css; charset=utf-8','.mjs':'text/javascript; charset=utf-8','.js':'text/javascript; charset=utf-8','.json':'application/json; charset=utf-8','.wasm':'application/wasm','.svg':'image/svg+xml','.png':'image/png','.jpg':'image/jpeg','.jpeg':'image/jpeg','.gif':'image/gif','.zip':'application/zip','.bin':'application/octet-stream','.uf2':'application/octet-stream','.md':'text/markdown; charset=utf-8','.txt':'text/plain; charset=utf-8'};
const assets={};let bytes=0;
async function walk(dir){for(const entry of (await readdir(dir,{withFileTypes:true})).sort((a,b)=>a.name.localeCompare(b.name))){if(entry.name==='.DS_Store')continue;if(entry.isSymbolicLink())throw Error('Asset symlinks are not allowed');const path=join(dir,entry.name);if(entry.isDirectory())await walk(path);else if(entry.isFile()){const data=await readFile(path),name='/'+relative(assetsDir,path).split('\\').join('/');const ext=/\.[^./]+$/.exec(name)?.[0].toLowerCase();assets[name]={data:data.toString('base64'),type:types[ext]||'text/plain; charset=utf-8',bytes:data.length,sha256:createHash('sha256').update(data).digest('hex')};bytes+=data.length;}}}
await walk(assetsDir);
for(const file of [...catalog.releases,...catalog.legacy]){if(!assets[file.path]||assets[file.path].sha256!==file.sha256||assets[file.path].bytes!==file.bytes)throw Error('Download catalogue integrity mismatch: '+file.id);}
const source=await readFile(join(here,'worker.mjs'),'utf8');
const code=source+'\nconst SITE_CONFIG='+JSON.stringify({...project,catalog,assets})+';\nexport default createSiteWorker(SITE_CONFIG);\n';
if(Buffer.byteLength(code)>64*1024*1024)throw Error('Worker exceeds64MiB bundle limit');
const temp=join(cwd,'.web-lab-output');await mkdir(join(temp,'server'),{recursive:true});await writeFile(join(temp,'server/index.js'),code);
await mkdir(join(temp,'.openai'),{recursive:true});await writeFile(join(temp,'.openai/hosting.json'),JSON.stringify(manifest,null,2)+'\n');
const result={result:'PASS',project:project.project,asset_count:Object.keys(assets).length,asset_bytes:bytes,worker_bytes:Buffer.byteLength(code),worker_limit_bytes:64*1024*1024,embedded_assets:Object.fromEntries(Object.entries(assets).map(([p,v])=>[p,{sha256:v.sha256,bytes:v.bytes,type:v.type}]))};
await mkdir(join(cwd,'build'),{recursive:true});await writeFile(join(cwd,'build/ASSET-MANIFEST.json'),JSON.stringify(result,null,2)+'\n');
try{await stat(outDir);await rename(outDir,join(cwd,'build','previous-output-'+Date.now()));}catch(error){if(error.code!=='ENOENT')throw error;}
await rename(temp,outDir);
console.log(JSON.stringify({result:'PASS',project:result.project,asset_count:result.asset_count,asset_bytes:bytes,worker_bytes:result.worker_bytes}));
