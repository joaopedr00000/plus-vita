// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_LAN_LOGIN_PAGE_H
#define PV_LAN_LOGIN_PAGE_H
static const char pv_lan_page[] =
"<!doctype html><html lang=en><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>Plus-Vita local sign-in</title><style>body{max-width:480px;margin:24px auto;padding:16px;font:17px system-ui,sans-serif;background:#101c36;color:white}a,button{display:block;padding:14px;margin:12px 0;border-radius:8px;background:#2756a0;color:white;text-decoration:none;text-align:center}textarea{box-sizing:border-box;width:100%;font:inherit;padding:10px}button{width:100%;border:0;font:inherit}</style>"
"<h1>Plus-Vita</h1><p>Sign in only on the official Sony website.</p>"
"<a target=_blank rel='noopener noreferrer' href='https://www.playstation.com/'>1. Sign in at PlayStation</a>"
"<a target=_blank rel='noopener noreferrer' href='https://ca.account.sony.com/api/v1/ssocookie'>2. Open Sony token response</a>"
"<label for=t>3. Copy and paste the NPSSO response</label><textarea id=t rows=4 maxlength=512 autocomplete=off autocorrect=off autocapitalize=off spellcheck=false></textarea>"
"<p><small>Local HTTP is not encrypted. Use trusted private Wi-Fi. Never paste a password or verification code. Clear the clipboard after connecting.</small></p>"
"<button id=b type=button>Connect to Vita</button><p id=s role=status></p>"
"<script>'use strict';let code=location.hash.slice(1);history.replaceState(null,'',location.pathname);"
"const t=document.getElementById('t'),b=document.getElementById('b'),s=document.getElementById('s');"
"function clear(){t.value='';code='';b.disabled=true;}"
"if(!/^[a-f0-9]{32}$/.test(code)){clear();s.textContent='Scan a fresh QR on Vita.';}"
"b.onclick=async()=>{let v=t.value.trim();if(v.startsWith('{')){try{v=JSON.parse(v).npsso;}catch{v='';}}"
"if(typeof v!=='string'||!/^[A-Za-z0-9]{64}$/.test(v)){s.textContent='Paste the valid Sony NPSSO response, not a password.';return;}"
"b.disabled=true;try{const r=await fetch('/connect',{method:'POST',headers:{'Content-Type':'text/plain'},body:code+v,cache:'no-store',credentials:'omit',redirect:'error'});"
"if(!r.ok)throw Error();clear();s.textContent='Sent. Return to the Vita and confirm.';}catch{clear();s.textContent='Pairing failed. Scan a fresh QR.';}finally{v='';t.value='';}};"
"</script></html>";
#endif
