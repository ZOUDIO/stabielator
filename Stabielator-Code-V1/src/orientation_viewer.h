/**
 * @file orientation_viewer.h
 * @brief Canvas-liveweergave van IMU-quaternions met interpolatie en versheidsbewaking.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include <Arduino.h>

// Frameworkvrije Canvas-weergave. Quaternionvolgorde overal [w,x,y,z].
static constexpr char ORIENTATION_VIEWER_SCRIPT[] PROGMEM = R"JS(
'use strict';
const orientationMath = {
  normalize(q) {
    if (!Array.isArray(q) || q.length!==4 || !q.every(Number.isFinite)) return null;
    const n=Math.hypot(...q);
    return n>1e-8 ? q.map(v=>v/n) : null;
  },
  slerp(a,b,t) {
    let dot=a.reduce((sum,v,i)=>sum+v*b[i],0);
    if(dot<0){b=b.map(v=>-v);dot=-dot}
    dot=Math.min(1,dot);t=Math.max(0,Math.min(1,t));
    if(dot>.9995)return this.normalize(a.map((v,i)=>v+t*(b[i]-v)));
    const angle=Math.acos(dot),sin=Math.sin(angle);
    const p=Math.sin((1-t)*angle)/sin,r=Math.sin(t*angle)/sin;
    return a.map((v,i)=>p*v+r*b[i]);
  },
  rotate(q,v) {
    const [w,x,y,z]=q,[a,b,c]=v;
    const tx=2*(y*c-z*b),ty=2*(z*a-x*c),tz=2*(x*b-y*a);
    return [a+w*tx+y*tz-z*ty,b+w*ty+z*tx-x*tz,c+w*tz+x*ty-y*tx];
  }
};
(() => {
  const $=id=>document.getElementById(id),panel=$('orientationCard');
  if(!panel)return;
  const canvas=$('orientationCanvas'),ctx=canvas.getContext('2d');
  const mode=$('orientationMode'),state=$('orientationState'),rates=$('orientationRates');
  const advanced=location.pathname==='/advanced'||location.pathname==='/wifi';
  let debug=false,running=false,inFlight=false,timer=null,frame=null,controller=null,epoch=0;
  let goal=[1,0,0,0],current=goal.slice(),havePose=false,good=false;
  let lastKey=null,lastData=null,lastDevice=null,generation=null,deadline=0;
  let started=0,lastFrame=0,lastStats=0,measurements=[],frames=[];
  const vertices=[[-1,-1,-1],[1,-1,-1],[1,1,-1],[-1,1,-1],[-1,-1,1],[1,-1,1],[1,1,1],[-1,1,1]];
  const faces=[
    {ids:[0,3,2,1],normal:[0,0,-1],color:'#9ba9af'},
    {ids:[4,5,6,7],normal:[0,0,1],color:'#b4d1ef',label:'+Z'},
    {ids:[0,1,5,4],normal:[0,-1,0],color:'#a8c1b8'},
    {ids:[3,7,6,2],normal:[0,1,0],color:'#64ad96',label:'+Y / VOOR'},
    {ids:[1,2,6,5],normal:[1,0,0],color:'#e5b0a7',label:'+X'},
    {ids:[0,4,7,3],normal:[-1,0,0],color:'#c6b8b5'}
  ];
  const dot=(a,b)=>a.reduce((sum,v,i)=>sum+v*b[i],0);
  // Vaste camera op (+X,+Y,+Z), Z omhoog; geen Euler-omzetting of asverwisseling.
  const right=[.832050,-.554700,0],up=[-.269069,-.403604,.874475],eye=[.485071,.727607,.485071];
  function draw() {
    if(!ctx)return;
    const width=canvas.clientWidth||600,height=canvas.clientHeight||330,dpr=Math.min(devicePixelRatio||1,2);
    if(canvas.width!==Math.round(width*dpr)||canvas.height!==Math.round(height*dpr)){
      canvas.width=Math.round(width*dpr);canvas.height=Math.round(height*dpr);
    }
    ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,width,height);
    const scale=Math.min(width/5.8,height/4.9),q=good?current:[1,0,0,0];
    const project=v=>{const f=scale/(1-dot(v,eye)/8);return [width/2+dot(v,right)*f,height*.54-dot(v,up)*f]};
    ctx.fillStyle='#193b3910';ctx.beginPath();ctx.ellipse(width/2,height*.86,scale*1.5,scale*.22,0,0,Math.PI*2);ctx.fill();
    const rotated=vertices.map(v=>orientationMath.rotate(q,v)),points=rotated.map(project);
    const sorted=faces.map(f=>({...f,depth:f.ids.reduce((sum,i)=>sum+dot(rotated[i],eye),0)/4})).sort((a,b)=>a.depth-b.depth);
    for(const f of sorted){
      ctx.beginPath();f.ids.forEach((index,i)=>{const [x,y]=points[index];i?ctx.lineTo(x,y):ctx.moveTo(x,y)});ctx.closePath();
      ctx.fillStyle=good?f.color:'#d6dfdb';ctx.fill();ctx.strokeStyle=good?'#305e5666':'#718a7f55';ctx.lineWidth=1.5;ctx.stroke();
      if(good&&f.label&&dot(orientationMath.rotate(q,f.normal),eye)>0){
        const center=f.ids.reduce((v,i)=>v.map((n,k)=>n+rotated[i][k]/4),[0,0,0]);
        const [x,y]=project(center);ctx.fillStyle='#193b39';ctx.font='bold 12px system-ui';ctx.textAlign='center';ctx.fillText(f.label,x,y+4);
      }
    }
    if(good){
      const origin=project([0,0,0]);
      for(const [axis,label,color]of [[[1.85,0,0],'+X','#ac4439'],[[0,1.85,0],'+Y','#147959'],[[0,0,1.85],'+Z','#2867b7']]){
        const end=project(orientationMath.rotate(q,axis));ctx.strokeStyle=color;ctx.lineWidth=2.5;ctx.beginPath();ctx.moveTo(...origin);ctx.lineTo(...end);ctx.stroke();
        ctx.fillStyle=color;ctx.font='bold 13px system-ui';ctx.textAlign='center';ctx.fillText(label,end[0],end[1]-8);
      }
    }else{
      ctx.fillStyle='#536e65';ctx.font='13px system-ui';ctx.textAlign='center';ctx.fillText('Geen actuele houding',width/2,height-18);
    }
  }
  function stats(now) {
    measurements=measurements.filter(t=>now-t<1000);frames=frames.filter(t=>now-t<1000);
    const seconds=Math.max(.001,Math.min(1,(now-started)/1000));
    rates.textContent=`${good?Math.round(measurements.length/seconds):0} nieuwe metingen/s · ${good?Math.round(frames.length/seconds):0} animatiebeelden/s`;
  }
  function invalidate(text) {
    const changed=good||havePose||state.textContent!==text;
    good=false;havePose=false;measurements=[];frames=[];
    if(frame!==null){cancelAnimationFrame(frame);frame=null}
    state.textContent=text;state.style.color='#a44e44';stats(performance.now());if(changed)draw();
  }
  function animate(now) {
    frame=null;
    if(!running)return;
    if(now>=deadline){invalidate('Geen nieuwe, actuele IMU-metingen.');return}
    if(now-lastFrame>=15.5){
      const elapsed=Math.min(100,now-lastFrame);lastFrame=now;
      current=orientationMath.slerp(current,goal,1-Math.exp(-elapsed/35));
      draw();frames.push(now);
      if(now-lastStats>=500){lastStats=now;stats(now)}
    }
    frame=requestAnimationFrame(animate);
  }
  function pose(data) {
    const q=orientationMath.normalize(mode.value==='relative'?data.relative:data.q);
    if(!data.valid||!q){invalidate(mode.value==='relative'&&data.valid?'Geen gekalibreerde referentie beschikbaar.':'IMU-data ontbreekt of is verouderd.');return}
    if(performance.now()>=deadline){invalidate('Geen nieuwe, actuele IMU-metingen.');return}
    goal=q;
    if(!havePose){current=q.slice();havePose=true}
    good=true;state.style.color='';
    state.textContent=mode.value==='relative'?'Houding ten opzichte van de gekalibreerde referentie.':'Absolute houding in het IMU-wereldkader.';
    if(frame===null){lastFrame=performance.now()-17;frame=requestAnimationFrame(animate)}
  }
  async function poll(token) {
    if(!running||token!==epoch)return;
    if(inFlight){timer=setTimeout(()=>poll(token),5);return}
    inFlight=true;
    const begin=performance.now(),activeController=new AbortController();controller=activeController;
    const timeout=setTimeout(()=>activeController.abort(),1200);
    try{
      const response=await fetch('/api/orientation',{cache:'no-store',signal:activeController.signal});
      if(!response.ok)throw Error('HTTP '+response.status);
      const data=await response.json(),now=performance.now();
      if(!running||token!==epoch)return;
      if(!data.debug){setDebug(false);return}
      $('orientationSource').textContent='Live IMU';
      if(generation!==data.generation || (lastDevice!==null&&((data.device_ms-lastDevice)>>>0)>0x80000000)){
        lastKey=null;havePose=false;measurements=[];frames=[];generation=data.generation;
      }
      lastDevice=data.device_ms;lastData=data;
      const key=`${data.generation}:${data.seq}:${data.sample_ms}`;
      const freshUntil=begin+250-Number(data.age_ms);
      if(key!==lastKey){
        lastKey=key;deadline=freshUntil;
        if(data.valid&&Number.isFinite(deadline)&&deadline>now)measurements.push(now);
      }else deadline=Math.min(deadline,freshUntil);
      if(!Number.isFinite(deadline)){invalidate('Ongeldige meettijd ontvangen.');return}
      pose(data);
    }catch(error){
      if(running&&token===epoch)invalidate('Geen verbinding met de live IMU-weergave.');
    }finally{
      clearTimeout(timeout);inFlight=false;
      if(token===epoch){
        controller=null;
        // Een verzoek tegelijk. Streef naar 30 Hz; bij een trage verbinding geen inhaalverzoeken.
        if(running)timer=setTimeout(()=>poll(token),Math.max(1,1000/30-(performance.now()-begin)));
      }
    }
  }
  function sync() {
    const wanted=advanced&&debug&&panel.open&&!document.hidden;
    if(wanted===running)return;
    running=wanted;++epoch;
    if(timer!==null){clearTimeout(timer);timer=null}
    if(frame!==null){cancelAnimationFrame(frame);frame=null}
    if(controller){controller.abort();controller=null}
    measurements=[];frames=[];lastKey=null;lastData=null;lastDevice=null;generation=null;
    started=performance.now();lastStats=started;deadline=0;havePose=false;good=false;
    if(running){state.textContent='Wachten op actuele IMU-data...';poll(epoch)}
    else{state.textContent='Liveweergave gepauzeerd.';state.style.color='';stats(started);draw()}
  }
  function setDebug(enabled) {
    debug=!!enabled;panel.hidden=!advanced||!debug;sync();
  }
  panel.addEventListener('toggle',sync);
  mode.addEventListener('change',()=>{havePose=false;if(lastData&&running)pose(lastData)});
  document.addEventListener('visibilitychange',sync);
  window.addEventListener('pagehide',()=>{debug=false;sync()});
  window.addEventListener('resize',()=>{if(running)draw()});
  window.StabielatorCube={setDebug};
  if(!ctx){state.textContent='Canvas wordt niet ondersteund door deze browser.';return}
  setDebug(typeof loadedSettings!=='undefined'&&!!loadedSettings?.debugEnabled);
})();
)JS";
