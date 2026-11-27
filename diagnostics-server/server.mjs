import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import {randomUUID,createHash,timingSafeEqual} from 'node:crypto';
import {pathToFileURL} from 'node:url';
const APPS=new Set(['vitaplus','greenvita','vitatwitch','xcloud-vita','ps2-vita','ponte-live','protocolo-zero','kaios-camera']);
const EVENTS=new Set(['startup','authentication','catalogue','stream_start','stream_stop','network','video','audio','decoder','input','update_download','update_install','crash','storage','render','unknown']);
const METRICS=new Set(['rtt_ms','fps','video_queue_ms','audio_queue_ms','packet_loss','decode_ms','buffer_ms','bitrate_kbps','dropped_frames','audio_underruns','http_status','native_error']);
const LIMIT=65536, RETENTION=30*86400000, MAX_REPORTS=10000;
const safeInt=(n)=>Number.isSafeInteger(n)&&Math.abs(n)<=2147483647;
const obj=(x)=>x&&typeof x==='object'&&!Array.isArray(x);
const keys=(x,allowed)=>obj(x)&&Object.keys(x).every(k=>allowed.includes(k));
export function validate(x){
 if(!keys(x,['schema','app','version','events','metrics'])||x.schema!==1||!APPS.has(x.app)||typeof x.version!=='string'||!/^\d{1,3}\.\d{1,3}(?:\.\d{1,3})?(?:[-.]?(?:b|r|rc)\d{1,3})?$/i.test(x.version)||!Array.isArray(x.events)||x.events.length<1||x.events.length>256||!obj(x.metrics)||Object.keys(x.metrics).length>METRICS.size)throw Error('invalid report');
 for(const e of x.events)if(!keys(e,['kind','code','elapsed_ms'])||!EVENTS.has(e.kind)||!safeInt(e.code)||!safeInt(e.elapsed_ms)||e.elapsed_ms<0)throw Error('invalid event');
 for(const [k,v] of Object.entries(x.metrics))if(!METRICS.has(k)||!safeInt(v))throw Error('invalid metric');
 return x;
}
const digest=s=>createHash('sha256').update(s).digest();
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
export function makeServer({dir,password}){
 if(typeof password!=='string'||password.length<24)throw Error('ADMIN_PASSWORD requires at least 24 characters');
 fs.mkdirSync(dir,{recursive:true,mode:0o700});
 const expected=digest('Basic '+Buffer.from('owner:'+password).toString('base64'));
 const rate=new Map(); let global={at:0,n:0};
 const files=()=>fs.readdirSync(dir).filter(n=>/^[0-9a-f-]{36}\.json$/.test(n));
 function prune(){const now=Date.now();for(const n of files()){const p=path.join(dir,n);if(now-fs.statSync(p).mtimeMs>RETENTION)fs.unlinkSync(p);}}
 prune();const timer=setInterval(()=>{try{prune()}catch{}},3600000);timer.unref();
 function send(res,status,data,type='application/json; charset=utf-8'){res.writeHead(status,{'Content-Type':type,'Cache-Control':'no-store','X-Content-Type-Options':'nosniff','Referrer-Policy':'no-referrer','Content-Security-Policy':"default-src 'none'; style-src 'unsafe-inline'; form-action 'self'; frame-ancestors 'none'"});res.end(type.startsWith('application/json')?JSON.stringify(data):data);}
 function authorized(req){return timingSafeEqual(expected,digest(req.headers.authorization||''));}
 function limited(req){const now=Date.now(), ip=req.socket.remoteAddress||'unknown';if(now-global.at>=60000)global={at:now,n:0};if(++global.n>60)return true;for(const [k,v] of rate)if(now-v.at>=60000)rate.delete(k);if(!rate.has(ip)&&rate.size>=4096)return true;const v=rate.get(ip)||{at:now,n:0};v.n++;rate.set(ip,v);return v.n>20;}
 const server=http.createServer(async(req,res)=>{
  try{
   const u=new URL(req.url,'http://localhost');
   if(req.method==='GET'&&u.pathname==='/health')return send(res,200,{ok:true});
   if(req.method==='POST'&&u.pathname==='/v1/reports'){
    if(limited(req))return send(res,429,{error:'rate limited'});
    if(!/^application\/json(?:\s*;|$)/i.test(req.headers['content-type']||''))return send(res,415,{error:'JSON required'});
    const declared=Number(req.headers['content-length']);if(!Number.isSafeInteger(declared)||declared<1||declared>LIMIT)return send(res,413,{error:'invalid size'});
    let size=0;const chunks=[];for await(const chunk of req){size+=chunk.length;if(size>LIMIT){req.destroy();return;}chunks.push(chunk);}
    let report;try{report=validate(JSON.parse(Buffer.concat(chunks).toString('utf8')))}catch{return send(res,400,{error:'invalid report'});}
    if(files().length>=MAX_REPORTS)return send(res,503,{error:'storage full'});
    const id=randomUUID(), record={id,received_at:new Date().toISOString(),...report};
    const dest=path.join(dir,id+'.json'),tmp=dest+'.tmp';fs.writeFileSync(tmp,JSON.stringify(record),{mode:0o600,flag:'wx'});fs.renameSync(tmp,dest);
    return send(res,201,{id});
   }
   if(req.method==='GET'&&(u.pathname==='/'||u.pathname==='/admin'||u.pathname.startsWith('/admin/report/'))){
    if(!authorized(req)){res.setHeader('WWW-Authenticate','Basic realm="Private diagnostics", charset="UTF-8"');return send(res,401,{error:'authentication required'});}
    if(u.pathname.startsWith('/admin/report/')){
     const id=u.pathname.slice('/admin/report/'.length);if(!/^[0-9a-f-]{36}$/.test(id))return send(res,404,{error:'not found'});
     const p=path.join(dir,id+'.json');if(!fs.existsSync(p))return send(res,404,{error:'not found'});
     return send(res,200,JSON.parse(fs.readFileSync(p,'utf8')));
    }
    const app=u.searchParams.get('app')||'',version=u.searchParams.get('version')||'';
    const records=files().map(n=>JSON.parse(fs.readFileSync(path.join(dir,n),'utf8'))).sort((a,b)=>b.received_at.localeCompare(a.received_at));
    const filtered=records.filter(r=>(!app||r.app===app)&&(!version||r.version===version));
    const page=Math.max(1,Math.min(1000,Number.parseInt(u.searchParams.get('page')||'1',10)||1));
    const rows=filtered.slice((page-1)*100,page*100).map(r=>'<tr><td>'+esc(r.received_at)+'</td><td>'+esc(r.app)+'</td><td>'+esc(r.version)+'</td><td>'+r.events.length+'</td><td><a href="/admin/report/'+r.id+'">Abrir JSON</a></td></tr>').join('');
    const opts=['',...APPS].map(a=>'<option value="'+esc(a)+'"'+(a===app?' selected':'')+'>'+esc(a||'Todos')+'</option>').join('');
    const q=new URLSearchParams({app,version});
    return send(res,200,'<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Central de diagnósticos</title><style>body{font:16px system-ui;max-width:1000px;margin:32px auto;padding:16px;background:#15171b;color:#eee}a{color:#9cc9ff}table{width:100%;border-collapse:collapse}td,th{padding:10px;text-align:left;border-bottom:1px solid #444}input,select,button{font:inherit;padding:8px;max-width:100%}.wrap{overflow:auto}</style><h1>Central privada de diagnósticos</h1><p>'+records.length+' relatórios armazenados · '+filtered.length+' encontrados · retenção de 30 dias</p><form method="get" action="/admin"><select name="app">'+opts+'</select> <input name="version" placeholder="Versão" value="'+esc(version)+'"> <button>Filtrar</button></form><div class="wrap"><table><thead><tr><th>Recebido UTC</th><th>Aplicativo</th><th>Versão</th><th>Eventos</th><th>Relatório</th></tr></thead><tbody>'+rows+'</tbody></table></div><p>Página '+page+' · '+(page>1?'<a href="/admin?'+esc(q.toString())+'&amp;page='+(page-1)+'">Anterior</a> ':'')+(page*100<filtered.length?'<a href="/admin?'+esc(q.toString())+'&amp;page='+(page+1)+'">Próxima</a>':'')+'</p></html>','text/html; charset=utf-8');
   }
   return send(res,404,{error:'not found'});
  }catch{if(!res.headersSent)send(res,500,{error:'server error'});else res.end();}
 });
 server.requestTimeout=20000;server.headersTimeout=10000;server.maxConnections=64;server.on('close',()=>clearInterval(timer));return server;
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href)makeServer({dir:process.env.REPORT_DIR||'/var/data/reports',password:process.env.ADMIN_PASSWORD}).listen(Number(process.env.PORT||10000),'0.0.0.0');
