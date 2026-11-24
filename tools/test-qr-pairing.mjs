// SPDX-License-Identifier: AGPL-3.0-only
import fs from 'node:fs';
import vm from 'node:vm';
import {webcrypto,createHash} from 'node:crypto';
import assert from 'node:assert/strict';
const html=fs.readFileSync('release/qr-pairing.html','utf8');
const scripts=[...html.matchAll(/<script>([\s\S]*?)<\/script>/g)];
assert.equal(scripts.length,2);
const entropy=Buffer.from(Array.from({length:32},(_,i)=>i+1));
const makeKey=device=>createHash('sha256').update('PlusVitaQR3-key').update(entropy).update(device).digest('hex');
const a=makeKey(Buffer.from([2,0,0,0,0,1])),b=makeKey(Buffer.from([2,0,0,0,0,2]));
assert.notEqual(a,b);
const canvas={getContext:()=>({fillRect(){}})};
const fields={key:{value:''},token:{value:''},qr:canvas,status:{},make:{},clear:{}};
const context=vm.createContext({document:{getElementById:id=>fields[id]},crypto:webcrypto,TextEncoder,btoa,addEventListener(){},setTimeout(){return 1},clearTimeout(){}});
vm.runInContext(scripts[0][1],context);
vm.runInContext('const original=qrcode; var payload=""; qrcode=(...args)=>{const qr=original(...args),add=qr.addData; qr.addData=(s)=>{payload=s;return add(s);};return qr;}',context);
vm.runInContext(scripts[1][1],context);
async function generate(key){
  fields.key.value=key;fields.token.value='Z'.repeat(64);await fields.make.onclick();
  assert.equal(fields.key.value,'');assert.equal(fields.token.value,'');assert.equal(canvas.hidden,false);
  const payload=vm.runInContext('payload',context);
  assert.equal(payload.length,172);assert.ok(payload.startsWith('PV3:'));return payload;
}
const p=await generate(a),q=await generate(b),r=await generate(a);
assert.notEqual(p,q);assert.notEqual(p,r);
const id=x=>Buffer.from(x.slice(4),'base64').subarray(0,32).toString('hex');
assert.notEqual(id(p),id(q));assert.equal(id(p),id(r));
console.log(p+'\n'+q);
