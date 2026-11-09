/**
 * @file web_ui.h
 * @brief Basis- en geavanceerde webbediening met opgeslagen pauze en handmatige GUI-tests.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
static constexpr char STABILIZER_PAGE[] = R"HTML(<!doctype html>
<html lang="nl"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Stabielator</title>
<style>
:root{color-scheme:light;--ink:#193b39;--muted:#667c78;--line:#dce7e2;--accent:#257b66;--paper:#fff;--bg:#f1f5f1}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:14px/1.45 "Segoe UI",system-ui,sans-serif}
.shell{max-width:1280px;margin:auto;padding:22px 30px 14px}header{display:flex;align-items:center;justify-content:space-between;gap:24px;margin-bottom:18px}
.brand{display:flex;align-items:center;gap:12px}.mark{width:38px;height:38px;display:grid;place-items:center;background:var(--ink);color:#c5ebc3;border-radius:13px;font-size:24px}
h1{font-size:21px;letter-spacing:-.7px;line-height:1.15;margin:0}h2{font-size:16px;letter-spacing:-.25px;margin:0 0 14px}p{margin:0}.subtitle,.hint{color:var(--muted);font-size:12px}.subtitle{margin-top:3px}
nav{display:flex;background:#e4ece6;padding:4px;border-radius:11px;gap:3px}nav a{padding:8px 20px;border-radius:8px;color:var(--muted);text-decoration:none;font-weight:600;font-size:13px}nav a[aria-current=page]{color:var(--ink);background:white;box-shadow:0 2px 7px #193b3910}
.summary{display:grid;grid-template-columns:1fr .75fr 1.65fr;gap:12px;margin-bottom:16px}.metric{background:#e5eee4;padding:13px 17px;border:1px solid #dae6d9;border-radius:13px;min-width:0}.eyebrow{font-size:10px;text-transform:uppercase;letter-spacing:1.3px;color:var(--muted);display:block;margin-bottom:4px}
.metric strong{font-size:17px;font-weight:600;display:block}.metric small{color:var(--muted);font-size:11px}.metric:last-child strong{font-size:15px}
main{display:grid;grid-template-columns:1.25fr 1fr;gap:16px}.card{border:1px solid var(--line);background:var(--paper);padding:20px;border-radius:15px;min-width:0}.card-head{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-bottom:16px}.card-head h2{margin:0}
.tag{font-size:10px;letter-spacing:.5px;color:var(--accent);background:#edf6ee;border-radius:5px;padding:4px 7px;white-space:nowrap}
.fields{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px 20px}label{display:block;min-width:0;font-size:12px;color:var(--muted)}label span{display:block;margin-bottom:5px}
input,select{font:inherit;font-size:13px;display:block;width:100%;min-width:0;height:36px;padding:6px 10px;border:1px solid var(--line);border-radius:7px;background:#fafcf9;color:var(--ink)}input:focus,select:focus{outline:2px solid #80b9a5;outline-offset:1px}
button{font:600 12px "Segoe UI",system-ui;cursor:pointer;padding:10px 14px;border:1px solid var(--line);border-radius:8px;color:var(--ink);background:white;min-height:36px;white-space:normal}button.primary{background:var(--accent);color:white;border-color:var(--accent)}button.danger{color:#a44e44;background:#fffaf8;border-color:#ead4cf}button:disabled{opacity:.45;cursor:default}button:hover:not(:disabled){filter:brightness(.96)}
.actions{display:flex;gap:8px;flex-wrap:wrap}.form-foot{display:flex;align-items:center;justify-content:space-between;gap:16px;margin-top:20px}.form-foot .hint{max-width:350px}.rule{height:1px;background:var(--line);margin:19px 0}.test-row{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin:10px 0 12px}.control-note{margin:10px 0 15px}.test-foot{display:flex;align-items:center;gap:12px}.test-foot .hint{font-size:11px}
.basic-summary{grid-template-columns:1fr 1fr}#wifiAdvanced{grid-column:1/-1}summary{cursor:pointer;font-weight:600}.wifi-grid{display:grid;grid-template-columns:1fr 1fr;gap:24px;margin-top:20px}#frequencyInfo{margin-top:14px}.advanced-card{grid-column:1/-1}.advanced-card .fields{grid-template-columns:repeat(4,minmax(0,1fr));gap:12px 20px}.advanced-card .form-foot{margin-top:16px}.diagnostic-link{border:0;padding:0;background:none;color:var(--accent);font-size:12px;min-height:0}.diagnostic-link:hover{text-decoration:underline}
dl{display:grid;grid-template-columns:110px 1fr;gap:12px;margin:0;font-size:13px}dt{color:var(--muted)}dd{margin:0;overflow-wrap:anywhere}.wifi-hint{margin-top:18px}.password-row{margin-top:15px}.check{display:flex;align-items:center;gap:7px;font-size:11px;margin-top:8px}.check input{height:14px;width:14px}.wifi-actions{margin-top:20px}.wifi-help{margin-top:15px;font-size:12px;color:var(--muted)}
footer{display:flex;justify-content:space-between;gap:20px;align-items:flex-start;margin-top:13px;color:var(--muted);font-size:11px;min-height:32px}#message{color:var(--accent);max-width:80ch;overflow-wrap:anywhere}footer .live{white-space:nowrap}footer .live:before{content:'';display:inline-block;background:#5f9b77;width:5px;height:5px;border-radius:50%;margin:0 6px 1px 0}
dialog{border:1px solid var(--line);border-radius:15px;padding:22px;width:min(740px,94vw);max-height:85vh;color:var(--ink)}dialog::backdrop{background:#17383266}dialog header{margin-bottom:12px}pre{margin:0;background:var(--bg);border-radius:8px;padding:14px;font:12px/1.45 Consolas,monospace;white-space:pre-wrap;overflow-wrap:anywhere;max-height:60vh;overflow:auto}dialog a{color:var(--accent);font-size:12px}
[hidden]{display:none!important}
@media(min-width:901px) and (max-height:740px){.shell{padding-top:15px;padding-bottom:10px}header{margin-bottom:12px}.summary{margin-bottom:12px}.metric{padding:10px 15px}.card{padding:17px}.card-head{margin-bottom:12px}.fields{gap:11px 18px}.advanced-card .fields{gap:9px 18px}.form-foot{margin-top:15px}.rule{margin:15px 0}}
@media(max-width:900px){.shell{padding:18px}header{gap:14px}nav a{padding:8px 12px}.summary{grid-template-columns:1fr 1fr}.metric:last-child{grid-column:1/-1}main{grid-template-columns:1fr}.advanced-card .fields{grid-template-columns:repeat(3,minmax(0,1fr))}}
@media(max-width:540px){.wifi-grid{grid-template-columns:1fr}.shell{padding:16px 12px}header{align-items:flex-start;flex-direction:column;gap:15px}nav{width:100%}nav a{flex:1;text-align:center}.card{padding:17px}.fields,.advanced-card .fields{grid-template-columns:repeat(2,minmax(0,1fr));gap:13px 12px}.metric strong{font-size:15px}.metric:last-child strong{font-size:13px}.form-foot{align-items:flex-start}.form-foot .hint{font-size:11px}.actions button{flex:1}.test-foot{align-items:flex-start;flex-direction:column}footer{gap:8px;flex-direction:column}.tag{font-size:9px}}
#developmentState{margin:10px 0}#orientationCard{margin-top:18px;padding-top:18px;border-top:1px solid var(--line)}
#orientationCard summary{display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap}
.orientation-tools{display:flex;align-items:end;gap:20px;flex-wrap:wrap;margin:16px 0 8px}.orientation-tools label{min-width:190px}
#orientationCanvas{display:block;width:100%;height:330px;border-radius:12px;background:linear-gradient(#f8fbfa,#eaf2ee);margin:12px 0}
.orientation-legend{display:flex;gap:16px;flex-wrap:wrap;font-size:12px}.orientation-legend span:nth-child(1){color:#a74036}.orientation-legend span:nth-child(2){color:#257b66}.orientation-legend span:nth-child(3){color:#295fa8}
#orientationState{margin-top:10px}#orientationRates{font-variant-numeric:tabular-nums}@media(max-width:540px){#orientationCanvas{height:290px}}
</style>
<div class="shell">
<header><div class="brand"><div class="mark" aria-hidden="true">s</div><div><h1>Stabielator</h1><p class="subtitle" id="pageTitle">Basis &middot; Houdingsfeedback</p></div></div>
<nav aria-label="Pagina's"><a href="/basis" data-page="basis">Basis</a><a href="/advanced" data-page="advanced">Geavanceerd</a></nav></header>
<div class="summary"><div class="metric"><span class="eyebrow">Apparaat</span><strong id="state">Verbinding maken</strong><small id="profile">Houdingsfeedback</small><small id="pauseState"></small></div><div class="metric"><span class="eyebrow">Accu</span><strong id="battery">&mdash;</strong><small>Geschat percentage &middot; 1-cels LiPo</small></div><div class="metric" id="angleMetric"><span class="eyebrow">Kanteling ten opzichte van referentie</span><strong id="angles">Hoeken laden...</strong><small id="motors">Motorstatus laden...</small></div></div>
<main>
<section class="card" id="settingsCard"><div class="card-head"><h2 id="settingsTitle">Basisinstellingen</h2><span class="tag">Bewaard op apparaat</span></div>
<form id="settings"><div class="fields" id="fields"></div><div class="form-foot"><p class="hint" id="settingsHint">0 minuten = inactiviteitstimer uit.</p><button class="primary" id="save" disabled>Opslaan</button></div></form></section>
<section class="card" id="controlCard"><div class="card-head"><h2>Bediening</h2><span class="tag">Live</span></div><p class="hint control-note">Stoppen wordt bewaard, ook na uit- en inschakelen. Pauze = rood. Hervatten geeft de houdingsfeedback vrij. Houd stil tijdens kalibratie.</p>
<div class="actions"><button class="primary" type="button" data-action="calibrate">Kalibreren</button><button class="danger" type="button" data-action="stop">Stoppen en bewaren</button><button type="button" data-action="resume">Hervatten</button></div>
<div class="rule"></div><h2>Trilpatroon testen</h2><div class="test-row"><label><span>Motor</span><select id="testMotor"><option value="1">Motor 1</option><option value="2">Motor 2</option><option value="3">Motor 3</option><option value="4">Motor 4</option></select></label><label><span>Testduur (ms)</span><input id="testDuration" type="number" min="20" max="3000" value="1800"></label></div>
<div class="test-foot"><button id="test" disabled>Test patroon</button><p class="hint">Gebruikt opgeslagen intensiteit en patroon.<br>Maximaal 3 seconden. Een handmatige test mag tijdelijk trillen in pauze; daarna blijft de opgeslagen pauze actief.</p></div></section>
<section class="card advanced-card" id="developmentCard" hidden><h2>Ontwikkeltest</h2><p class="hint">Teststand pauzeert automatische houdingsfeedback. IMU, accubeveiliging en inactiviteitsbewaking blijven werken. De keuze blijft bewaard na herstart. USB-debug en live 3D hebben een afzonderlijke keuze bij Geavanceerde instellingen en kunnen ook tijdens gewone feedback aan staan. Motorpulsen test je via Basis.</p><p class="hint" id="developmentState">Stand laden...</p><div class="actions"><button type="button" data-action="test_mode_on">Teststand aan</button><button type="button" data-action="test_mode_off">Teststand uit</button></div><p class="hint" id="frequencyInfo">De ERM-trilfrequentie volgt het toerental en de belasting. Stel de intensiteit en pulstijden in voor de gewenste feedback.</p>
<details id="orientationCard" open hidden><summary><span>Live 3D-houding</span><span class="tag" id="orientationSource">Debug</span></summary>
<div class="orientation-tools"><label><span>Houding weergeven</span><select id="orientationMode"><option value="absolute">Absoluut</option><option value="relative">Ten opzichte van referentie</option></select></label><p class="hint" id="orientationRates">0 nieuwe metingen/s &middot; 0 animatiebeelden/s</p></div>
<canvas id="orientationCanvas" role="img" aria-label="Live driedimensionale houding van de Stabielator"></canvas>
<div class="orientation-legend"><span>+X rood</span><span>+Y groen / voorzijde</span><span>+Z blauw / boven</span></div><p id="orientationState" role="status">Wachten op meetgegevens...</p><p class="hint">Sensorassen; de camera kijkt met Z omhoog. Controleer de voorkant (+Y) op de eigen montage. De animatie interpoleert tussen ontvangen metingen. Sluit dit paneel om de liveweergave te stoppen.</p></details></section>
<details class="card" id="wifiAdvanced" hidden><summary>Geavanceerd: verbinden met een wifinetwerk</summary><p class="hint wifi-hint">Normaal verbind je rechtstreeks met het instelnetwerk. Je kunt het AP apart uitschakelen. AP en routerwifi kunnen ook tegelijk aan blijven.</p><div class="wifi-grid">
<section id="wifiStatusCard"><h2>Netwerkstatus</h2><form id="apForm"><label><span>Eigen AP-netwerk</span><select id="apEnabled" name="enabled"><option value="1">Aan (standaard)</option><option value="0">Uit</option></select></label><button id="apSave" disabled>AP-keuze opslaan</button></form><p class="wifi-help">Keuze blijft bewaard na herstart. AP uit verbreekt een rechtstreekse verbinding. Gebruik daarna het routeradres of USB-serial: <code>ap on</code> (of <code>portal</code>). Firmware bijwerken kan via het AP.</p><span class="tag" id="networkState">Laden</span><dl><dt>Router</dt><dd id="wifiSsid">&mdash;</dd><dt>IP-adres</dt><dd id="wifiIp">&mdash;</dd><dt>Signaal</dt><dd id="wifiSignal">&mdash;</dd><dt>Instelnetwerk</dt><dd id="wifiAp">&mdash;</dd></dl></section>
<section id="wifiFormCard"><h2>Routerverbinding (optioneel)</h2><form id="wifiForm"><label><span>Netwerknaam (SSID)</span><input id="ssid" name="ssid" maxlength="32" required autocomplete="off"></label><label class="password-row"><span>Wachtwoord</span><input id="password" name="password" type="password" minlength="8" maxlength="63" autocomplete="new-password" placeholder="Leeg voor een open netwerk"></label><label class="check"><input id="showPassword" type="checkbox">Wachtwoord tonen</label><div class="wifi-actions"><button class="primary" id="wifiSave">Opslaan en verbinden</button></div></form><p class="wifi-help">De opgeslagen AP-keuze blijft gelden. Bij wisselen van radiokanaal kan de verbinding tijdelijk wegvallen.</p></section></div></details>
<section class="card advanced-card" id="firmwareCard" hidden><h2>Firmware bijwerken</h2><p>Installeer een Stabielator-update via het eigen instelnetwerk. De motoren stoppen tijdens het bijwerken; instellingen blijven bewaard.</p><p><a href="http://192.168.4.1/update">Firmwarebestand kiezen en bijwerken</a></p></section>
</main>
<footer><p id="message" role="status"></p><div class="live">Live &middot; 4 Hz <button class="diagnostic-link" id="openDiagnostics" type="button" hidden>&middot; Diagnose openen</button></div></footer>
</div>
<dialog id="diagnostics"><header><h2>Diagnose</h2><button id="closeDiagnostics">Sluiten</button></header><pre id="status">Status laden...</pre><p><a href="/api/status" target="_blank" rel="noopener">JSON-status openen</a></p></dialog>
<script>
const page=location.pathname==='/advanced'||location.pathname==='/wifi'?'advanced':'basis';
const $=id=>document.getElementById(id);
$('firmwareCard').hidden=page!=='advanced';
document.title=(page==='advanced'?'Geavanceerd':'Basis')+' | Stabielator';
$('pageTitle').textContent=page==='advanced'?'Geavanceerd \u00b7 Afstemming & diagnose':'Basis \u00b7 Houdingsfeedback';
document.querySelector(`[data-page="${page}"]`).setAttribute('aria-current','page');
$('controlCard').hidden=page!=='basis';$('angleMetric').hidden=page!=='advanced';$('developmentCard').hidden=$('wifiAdvanced').hidden=page!=='advanced';$('openDiagnostics').hidden=page!=='advanced';
if(page==='basis')document.querySelector('.summary').classList.add('basic-summary');
if(page==='advanced'){$('settingsCard').classList.add('advanced-card');$('settingsTitle').textContent='Geavanceerde instellingen';$('settingsHint').textContent='Max. trilduur: inclusief de uit-fasen. 0 = tot terugkeer. Elke richting krijgt een unieke motor.'}
if(location.hash==='#wifiAdvanced'||location.pathname==='/wifi')$('wifiAdvanced').open=true;
addEventListener('hashchange',()=>{if(location.hash==='#wifiAdvanced')$('wifiAdvanced').open=true});
const definitions=[
 ['onDegrees','Inschakelhoek (\u00b0)',.1,60,.1],['offDegrees','Uitschakelhoek (\u00b0)',0,59.9,.1],['strength','Trilintensiteit (%)',1,100,1],['pattern','Trilpatroon',[[0,'Continu'],[1,'Pulsen'],[2,'Dubbel Pulsen']]],['inactivityMinutes','Uit na stilstand (min)',0,1440,1],['ledEnabled','Status-led',[[1,'Aan'],[0,'Uit']]],['ledBrightness','Ledhelderheid (%)',0,100,1],
 ['bootDelayMs','Wachttijd opstarten (ms)',0,60000,1,true],['calibrationMs','Kalibratievenster (ms)',500,5000,1,true],['diagonal','Diagonale feedback',[[1,'Twee motoren'],[0,'Sterkste richting']],null,null,true],['pulseOnMs','Puls aan (ms)',20,1000,1,true],['pulseOffMs','Puls uit (ms)',20,5000,1,true],['maxFeedbackMs','Max. trilduur Pulsen (ms, 0 = uit)',0,60000,1,true],['maxContinuousOrDoubleMs','Max. trilduur Continu en Dubbel Pulsen (ms, 0 = uit)',0,60000,1,true],['activityDegrees','Bewegingsgrens stilstand (\u00b0)',.1,30,.1,true],['axisRotationDegrees','Asrotatie (\u00b0)',-180,180,.1,true],['batteryLowVolts','Accuwaarschuwing (V)',3.1,4.1,.05,true],['batteryCriticalVolts','Accu uitschakelen (V)',3,4,.05,true],['debugEnabled','USB-debug en live 3D',[[0,'Uit'],[1,'Aan']],null,null,true],...['X+','X-','Y+','Y-'].map((label,i)=>['motor'+i,'Motor voor '+label,[[0,'Motor 1'],[1,'Motor 2'],[2,'Motor 3'],[3,'Motor 4']],null,null,true])];
const visibleDefinitions=definitions.filter(d=>!!d[5]===(page==='advanced'));
for(const [name,title,min,max,step]of visibleDefinitions){const label=document.createElement('label'),caption=document.createElement('span');caption.textContent=title;label.append(caption);const input=document.createElement(Array.isArray(min)?'select':'input');input.name=name;input.required=true;if(Array.isArray(min)){for(const[value,text]of min){const option=document.createElement('option');option.value=value;option.textContent=text;input.append(option)}}else{input.type='number';input.min=min;input.max=max;input.step=step}label.append(input);$('fields').append(label)}
let loadedSettings=null,refreshing=false,wifiQueued=false,settingsRevision=0;
async function request(path,body){const r=await fetch(path,{method:body?'POST':'GET',body,cache:'no-store',signal:AbortSignal.timeout(4000)});if(!r.ok)throw Error(await r.text());return r.json()}
function updateDebugView(){++settingsRevision;window.StabielatorCube?.setDebug(!!loadedSettings?.debugEnabled)}
function message(text,error=false){$('message').textContent=text;$('message').style.color=error?'#a44e44':''}
async function loadSettings(){try{loadedSettings=await request('/api/settings');for(const[name]of visibleDefinitions)$('settings').elements.namedItem(name).value=name==='strength'?Math.round(loadedSettings.strength*100/127):loadedSettings[name];$('save').disabled=false;$('test').disabled=false;updateDebugView()}catch(error){message('Instellingen laden mislukt: '+error.message,true)}}
$('settings').addEventListener('submit',async event=>{event.preventDefault();$('save').disabled=true;try{const values=new FormData($('settings'));if(values.has('strength'))values.set('strength',Math.round(Number(values.get('strength'))*127/100));loadedSettings=await request('/api/settings',new URLSearchParams(values));updateDebugView();message('Instellingen opgeslagen.')}catch(error){message('Opslaan mislukt: '+error.message,true)}finally{$('save').disabled=false}});
for(const button of document.querySelectorAll('[data-action]'))button.addEventListener('click',async()=>{button.disabled=true;try{await request('/api/control',new URLSearchParams({action:button.dataset.action}));if(button.dataset.action.startsWith('test_mode_'))await loadSettings();message(button.dataset.action.startsWith('test_mode_')?'Ontwikkeltest '+(button.dataset.action==='test_mode_on'?'aan':'uit')+' opgeslagen.':button.dataset.action==='calibrate'?'Kalibratie aangevraagd. Houd het apparaat stil.':button.dataset.action==='stop'?'Pauze opgeslagen. De led is rood; ook na herstart blijft de houdingsfeedback uit.':button.dataset.action==='resume'?'Hervatten opgeslagen. Houdingsfeedback werkt wanneer Ontwikkeltest uit staat.':'Opdracht uitgevoerd.')}catch(error){message(error.message,true)}finally{button.disabled=false}});
$('test').addEventListener('click',async()=>{const duration=Number($('testDuration').value);if(!Number.isInteger(duration)||duration<20||duration>3000){message('Kies een testduur van 20 tot 3000 ms.',true);return}try{await request('/api/test',new URLSearchParams({motor:$('testMotor').value,duration}));message('Patroontest gestart.')}catch(error){message('Test mislukt: '+error.message,true)}});
$('showPassword').addEventListener('change',()=>{$('password').type=$('showPassword').checked?'text':'password'});
$('wifiForm').addEventListener('submit',async event=>{event.preventDefault();$('wifiSave').disabled=true;try{await request('/api/wifi',new URLSearchParams(new FormData($('wifiForm'))));wifiQueued=true;$('password').value='';message('Wifi opgeslagen. Verbinden gestart; de pagina kan tijdelijk wegvallen.')}catch(error){message('Wifi opslaan mislukt: '+error.message,true);$('wifiSave').disabled=false}});
$('apForm').addEventListener('submit',async event=>{event.preventDefault();$('apSave').disabled=true;try{await request('/api/ap',new URLSearchParams(new FormData($('apForm'))));message('AP-keuze opgeslagen. Bij uitschakelen kan deze verbinding wegvallen; gebruik het routeradres of USB-serial: ap on.')}catch(error){message('AP opslaan mislukt: '+error.message,true)}finally{$('apSave').disabled=false}});
let wifiLoaded=false;
async function refreshWifi(){const data=await request('/api/wifi');$('networkState').textContent=data.pending?'Verbinden':data.connected?'Verbonden':data.ap_active?'Instelnetwerk':'Niet verbonden';$('wifiSsid').textContent=data.ssid||'Nog niet ingesteld';$('wifiIp').textContent=data.connected?data.station_ip:'Geen routeradres';$('wifiSignal').textContent=data.connected?data.rssi+' dBm':'Niet beschikbaar';$('wifiAp').textContent=data.ap_active?data.ap_ssid+' \u00b7 '+data.ap_ip+(data.ap_hidden?' (verborgen)':' (zichtbaar)'):'Niet actief';if(!wifiLoaded){$('ssid').value=data.ssid||'';$('apEnabled').value=data.ap_enabled===false?'0':'1';$('apSave').disabled=false;wifiLoaded=true}if(wifiQueued&&!data.pending){wifiQueued=false;$('wifiSave').disabled=false;message(data.connected?'Wifi verbonden.':'Routerverbinding mislukt; probeer opnieuw. AP volgt de opgeslagen keuze.',!data.connected)}}
const states={waiting:'Wachten op kalibratie',calibrating:'Kalibreren \u00b7 houd stil',active:'Actief',imu_fault:'IMU-fout \u00b7 motoren uit',motor_fault:'Motorfout \u00b7 motoren uit',paused:'Gepauzeerd',inactive:'Uit na stilstand'};
async function refresh() {
  if(refreshing) return;
  refreshing=true;
  const revision=settingsRevision;
  try {
    const data=await request('/api/status');
    const angles=data.tilt_deg;
    if(revision===settingsRevision) {
      window.StabielatorCube?.setDebug(data.debug_enabled??!!loadedSettings?.debugEnabled);
    }
    const hardwareFault=data.state==='imu_fault'||data.state==='motor_fault';
    let stateText=states[data.state]||data.state;
    if(data.power.shutting_down) stateText='Voeding uitschakelen';
    else if(data.motor_stop_failed) stateText='Motorstop mislukt';
    else if(hardwareFault) stateText=states[data.state];
    else if(data.manual_test) stateText='Trilpatroon testen';
    else if(data.paused) stateText=data.pause_saved?'Gepauzeerd \u00b7 opgeslagen':'Gepauzeerd \u00b7 opslaan mislukt';
    else if(data.test_mode&&data.state==='paused') stateText='Teststand';
    $('state').textContent=stateText;
    $('state').style.color=(data.paused||hardwareFault||data.motor_stop_failed)?'#a44e44':'';
    let pauseText=data.paused?(data.pause_saved?'Pauze blijft opgeslagen':'Pauze opslaan mislukt'):'';
    if(data.paused && data.reference_valid) pauseText+=' \u00b7 referentie gereed';
    else if(data.paused && (data.state==='waiting'||data.state==='calibrating')) pauseText+=' \u00b7 '+states[data.state];
    $('pauseState').textContent=pauseText;
    $('developmentState').textContent=data.test_mode?'Teststand aan \u00b7 automatische feedback gepauzeerd':'Teststand uit \u00b7 automatische feedback beschikbaar';
    $('profile').textContent=page==='basis'?'Houdingsfeedback':(data.haptic_type||'')+' \u00b7 '+(data.haptic_motor_model||'Vier trilmotoren');
    $('angles').textContent=angles?`X ${angles.roll.toFixed(1)}\u00b0 / Y ${angles.pitch.toFixed(1)}\u00b0 / Yaw ${data.relative_orientation_deg.yaw.toFixed(1)}\u00b0`:'Referentiehoeken nog niet beschikbaar';
    $('battery').textContent=data.power.battery_v===null?'Niet beschikbaar':`${data.power.estimated_percent??'?'}% geschat / ${data.power.battery_v.toFixed(2)} V${data.power.critical?' \u00b7 kritiek':data.power.low?' \u00b7 laag':''}`;
    $('motors').textContent='Motor 1-4: '+data.motor_levels.join(' / ');
    $('status').textContent=JSON.stringify(data,null,2);
    if(page==='advanced') await refreshWifi();
  } catch(error) {
    $('state').textContent='Geen verbinding';
    $('angles').textContent='Hoeken niet beschikbaar';
    $('battery').textContent='Niet beschikbaar';
    $('motors').textContent='Motorstatus niet beschikbaar';
    if(page==='advanced') $('networkState').textContent=wifiQueued?'Opnieuw verbinden':'Geen verbinding';
  } finally {
    refreshing=false;
  }
}
$('openDiagnostics').addEventListener('click',()=>$('diagnostics').showModal());$('closeDiagnostics').addEventListener('click',()=>$('diagnostics').close());
if(page==='advanced'){const viewer=document.createElement('script');viewer.src='/orientation.js';document.head.append(viewer)}
loadSettings();refresh();setInterval(refresh,250);
</script></html>)HTML";
