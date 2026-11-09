/**
 * @file firmware_update_ui.h
 * @brief Webpagina voor bestandscontrole, firmware-upload en bevestiging na herstart.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
static constexpr char FIRMWARE_UPDATE_PAGE[] = R"HTML(<!doctype html>
<html lang="nl"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Firmware bijwerken | Stabielator</title>
<style>body{max-width:640px;margin:32px auto;padding:16px;font:16px/1.5 system-ui;color:#193b39;background:#f1f5f1}section{background:white;padding:24px;border-radius:15px}input,button,progress{display:block;margin:16px 0;max-width:100%;width:100%;box-sizing:border-box}button{padding:12px;background:#257b66;color:white;border:0;border-radius:8px;cursor:pointer}button:disabled{opacity:.5}#message{white-space:pre-wrap;overflow-wrap:anywhere}a{color:#257b66}</style>
<section><h1>Firmware bijwerken</h1><p id="current">Apparaatinformatie laden...</p>
<p>Kies het passende <b>Stabielator-…-v….stab.bin</b>-bestand. Instellingen blijven bewaard. De motoren stoppen tijdens de update. Houd de voeding aangesloten en blijf op het instelnetwerk.</p>
<form id="updateForm"><label for="file">Firmwarebestand</label><input id="file" type="file" accept=".stab.bin" required disabled><p id="selected"></p><button id="start" disabled>Controleren en bijwerken</button></form>
<progress id="progress" value="0" max="100" aria-label="Uploadvoortgang"></progress><p id="message" role="status" aria-live="polite"></p><a href="/advanced">Terug naar Geavanceerd</a></section>
<script>
const $=id=>document.getElementById(id);let info,busy=false,expectedVersion,reconnecting=false,confirmed=false;
function message(text,error=false){$('message').textContent=text;$('message').style.color=error?'#a44e44':''}
function text(bytes){return new TextDecoder().decode(bytes).split('\0')[0]}
async function inspect(file){
 if(!file||!file.name.endsWith('.stab.bin'))throw Error('Kies een .stab.bin-updatebestand.');
 const b=new Uint8Array(await file.slice(0,152).arrayBuffer());
 if(b.length!==152||text(b.slice(0,8))!=='STABOTA1')throw Error('Geen geldig Stabielator-updatebestand.');
 const target=text(b.slice(8,40)),version=text(b.slice(40,72)),size=new DataView(b.buffer).getUint32(72,true);
 if(target!==info.target)throw Error('Verkeerde hardwarevariant: '+target+'. Dit apparaat vereist '+info.target+'.');
 if(size<288||size>info.capacity||file.size!==size+128)throw Error('Bestand te groot, beschadigd of onvolledig.');
 if(b[128]!==233||b[140]!==5||b[141]!==0)throw Error('Geen ESP32-C3-applicatie.');
 expectedVersion=version;return 'Nieuwe versie '+version+' · '+size.toLocaleString('nl-NL')+' bytes · '+target;
}
$('file').onchange=async()=>{try{$('selected').textContent=await inspect($('file').files[0]);$('start').disabled=busy;message('Bestand herkend. Bij upload controleert het apparaat ook de volledige inhoud.')}catch(e){$('start').disabled=true;message(e.message,true)}};
$('updateForm').onsubmit=async e=>{e.preventDefault();if(busy)return;busy=true;$('start').disabled=true;$('file').disabled=true;
 try{reconnecting=false;confirmed=false;const file=$('file').files[0];await inspect(file);const form=new FormData();form.append('firmware',file);
 const xhr=new XMLHttpRequest();xhr.open('POST','/api/update');xhr.setRequestHeader('X-Stabielator-Token',info.token);xhr.timeout=180000;
 xhr.upload.onprogress=e=>{if(e.lengthComputable){const percent=Math.round(e.loaded/e.total*100);$('progress').value=percent;message(percent<100?'Uploaden: '+percent+'%':'Upload ontvangen. Inhoud controleren en activeren...');if(percent===100)reconnect()}};
 xhr.onload=()=>{if(confirmed)return;if(xhr.status!==200){failed(xhr.responseText||'Update geweigerd ('+xhr.status+').');return}message('Update geslaagd. Het apparaat start opnieuw. Opnieuw verbinden...');reconnect()};
 xhr.onerror=()=>{if(confirmed)return;message('Verbinding verbroken. Uitkomst controleren; verbind zo nodig opnieuw met het instelnetwerk.',true);reconnect()};
 xhr.ontimeout=()=>{if(confirmed)return;message('Upload timeout. Uitkomst controleren; verbind zo nodig opnieuw met het instelnetwerk.',true);reconnect()};
 xhr.onabort=()=>{if(!confirmed)failed('Upload afgebroken. Controleer de verbinding en probeer opnieuw.')};
 message('Upload starten; motoren worden gestopt...');xhr.send(form);
 }catch(e){failed(e.message)}
};
function failed(error){reconnecting=false;message(error+' Na een mislukte upload blijven de motoren gepauzeerd; hervat via Basis.',true);busy=false;$('file').disabled=false;$('start').disabled=false}
async function reconnect(){
 if(reconnecting)return;reconnecting=true;
 const started=Date.now(),previousToken=info.token;
 async function poll(){
  if(!reconnecting)return;
  try{const r=await fetch('/api/update',{cache:'no-store',signal:AbortSignal.timeout(8000)});if(r.ok){const next=await r.json();if(next.token!==previousToken){confirmed=true;reconnecting=false;message(next.version===expectedVersion?'Update geslaagd. Opnieuw verbonden met firmware v'+next.version+'.':'Apparaat verbonden met v'+next.version+'; verwacht was '+expectedVersion+'.',next.version!==expectedVersion);busy=false;$('file').value='';$('selected').textContent='';$('start').disabled=true;await init();return}}}catch(e){}
  if(Date.now()-started>90000){message('Herstart aangevraagd. Verbind opnieuw met het Stabielator-instelnetwerk en open deze pagina om de versie te controleren.',true);return}
  setTimeout(poll,3000);
 }
 setTimeout(poll,3000);
}
async function init(){try{const r=await fetch('/api/update',{cache:'no-store'});if(!r.ok)throw Error(await r.text());info=await r.json();$('current').textContent='Actieve firmware: v'+info.version+' · '+info.target+' · maximaal '+info.capacity.toLocaleString('nl-NL')+' bytes';if(!info.capacity)throw Error('Geen OTA-ruimte. Installeer de OTA-build eerst via USB.');if(info.busy)throw Error('Er loopt al een update; wacht tot de herstart.');$('file').disabled=false;if($('file').files.length)$('file').onchange()}catch(e){message(e.message,true)}}init();
</script></html>)HTML";
