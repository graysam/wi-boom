(()=>{
  const $=id=>document.getElementById(id);
  // Elements
  const ledWs=$("ledWs"), ledArmed=$("ledArmed"), apName=$("apName"), veil=$("veil"), info=$("infobar");
  const fire=$("fire"), arm=$("arm"), disarm=$("disarm");
  const count=$("count"), rate=$("rate"), repeats=$("repeats"), repSpa=$("repSpa"), continuous=$("continuous");
  const valCount=$("valCount"), valRate=$("valRate"), valReps=$("valReps"), valSpa=$("valSpa"), lblRepSpa=$("lblRepSpa");
  const timer=$("timer"), tmm=$("tmm"), tss=$("tss"), cdText=$("countdown");

  // State
  let ws=null, reconnectTimer=null;
  let armed=false, pulseActive=false, connected=false;
  let timeOffset=0; // serverNow - clientNow
  let lastDeadline=null; // ms epoch (server millis base) when timer ends

  function led(el, c, blink){ if(!el) return; el.className = 'led '+c+(blink?' blink':''); }
  function clamp(n,a,b){ return Math.max(a, Math.min(b, n)); }
  function mmssToMs(){ return (clamp(+tmm.value|0,0,59)*60 + clamp(+tss.value|0,0,59))*1000; }
  function updateVals(){
    valCount.textContent = count.value;
    valRate.textContent = rate.value;
    valReps.textContent = repeats.value;
    valSpa.textContent  = repSpa.value;
    repSpa.disabled = repeats.value <= 1 || continuous.checked;
    lblRepSpa.style.opacity = repSpa.disabled ? 0.4 : 1;
    const disableBurst = !!continuous.checked;
    [count,rate,repeats].forEach(el=>el.disabled = disableBurst);
  }

  function setArmed(on){
    armed=!!on;
    document.body.classList.toggle('armed', armed);
    if(armed){
      led($("ledArmed"),'amber',true);
      fire.disabled=false; fire.classList.add('enabled');
      [count,rate,repeats,repSpa,continuous,timer,tmm,tss].forEach(el=>el.disabled=true);
    }else{
      led($("ledArmed"),'red',false);
      fire.disabled=true; fire.classList.remove('enabled');
      [count,rate,repeats,repSpa,continuous,timer,tmm,tss].forEach(el=>el.disabled=false);
    }
  }

  function sendCfg(){
    if(!ws || ws.readyState!==1 || armed) return;
    ws.send(JSON.stringify({
      cmd:'cfg',
      count:+count.value,
      rateHz:+rate.value,
      repeats:+repeats.value,
      repSpa:+repSpa.value,
      continuous:!!continuous.checked,
      timer:!!timer.checked,
      timerMs:mmssToMs()
    }));
  }
  [count,rate,repeats,repSpa,continuous,timer,tmm,tss].forEach(el=>{
    el.addEventListener('input', ()=>{ updateVals(); sendCfg(); });
    el.addEventListener('change', ()=>{ updateVals(); sendCfg(); });
  });

  function fmtMS(ms){
    const sign = ms<0?'+':''; ms=Math.abs(ms)|0; const s=Math.floor(ms/1000); const m=Math.floor(s/60); const rs=s%60; return `${sign}${String(m).padStart(2,'0')}:${String(rs).padStart(2,'0')}`;
  }
  function tick(){
    if(lastDeadline!=null){
      const clientNow = performance.now();
      const serverNow = clientNow + timeOffset;
      const remaining = (lastDeadline - serverNow);
      cdText.textContent = fmtMS(remaining);
    } else {
      cdText.textContent = '';
    }
    requestAnimationFrame(tick);
  }
  requestAnimationFrame(tick);

  function timeSync(){
    if(!ws || ws.readyState!==1) return;
    const t0=performance.now();
    const handler = (ev)=>{
      try{ const m=JSON.parse(ev.data); if(m.type==='time'){ const t1=performance.now(); const rtt=(t1-t0); const midpoint=t0 + rtt/2; timeOffset = (m.nowMs - midpoint); ws.removeEventListener('message', handler); } }catch(e){}
    };
    ws.addEventListener('message', handler);
    ws.send(JSON.stringify({cmd:'timeSync'}));
  }

  arm.onclick = ()=>{ if(ws && ws.readyState===1) ws.send(JSON.stringify({cmd:'arm', on:true})); };
  disarm.onclick = ()=>{ if(ws && ws.readyState===1) ws.send(JSON.stringify({cmd:'arm', on:false})); };
  fire.onclick = ()=>{
    if(!ws || ws.readyState!==1) return;
    // Ensure cfg includes timer params just before firing
    sendCfg();
    ws.send(JSON.stringify({cmd:'fire'}));
  };

  function applyState(m){
    setArmed(m.armed);
    pulseActive = !!m.pulseActive;
    apName.textContent = m.apSSID || '';
    const c=m.cfg||{};
    count.value=c.count||1; rate.value=c.rateHz||10; repeats.value=c.repeats||1; repSpa.value=c.repSpa||500; continuous.checked=!!c.continuous; timer.checked=!!c.timer; const tMs=c.timerMs||10000; tmm.value = Math.floor((tMs/1000)/60); tss.value = Math.floor((tMs/1000)%60);
    updateVals();
    // Timer sync
    if(m.timer && m.timer.active){ lastDeadline = m.timer.deadlineMs; }
    else { lastDeadline = null; }
  }

  function connect(){
    const proto = location.protocol==='https:'?'wss':'ws';
    try{ ws && ws.close && ws.close(); }catch(e){}
    reconnectTimer && clearTimeout(reconnectTimer); reconnectTimer=null;
    led($("ledWs"),'amber',true); veil.classList.remove('hidden'); info.textContent='Connecting...';
    ws = new WebSocket(`${proto}://${location.host}/ws`);
    ws.onopen = ()=>{ connected=true; led($("ledWs"),'green',false); veil.classList.add('hidden'); info.textContent='Connected.'; timeSync(); };
    ws.onmessage = (ev)=>{ try{ const m=JSON.parse(ev.data); if(m.type==='state'){ applyState(m); } }catch(e){} };
    const schedule=()=>{ if(reconnectTimer) return; reconnectTimer=setTimeout(connect, 800); };
    ws.onerror = ()=>{ connected=false; led($("ledWs"),'red',false); veil.classList.remove('hidden'); info.textContent='Connection error. Retrying...'; schedule(); };
    ws.onclose = ()=>{ connected=false; led($("ledWs"),'red',false); veil.classList.remove('hidden'); info.textContent='Disconnected. Retrying...'; schedule(); };
  }

  // Prevent accidental zoom/scroll
  document.addEventListener('gesturestart', e=>e.preventDefault());
  document.addEventListener('dblclick', e=>e.preventDefault());
  document.addEventListener('touchmove', e=>{ if(e.scale && e.scale!==1) e.preventDefault(); }, {passive:false});

  updateVals();
  connect();
})();

  // Show IPs on status bar tap
  document.querySelector(".statusbar").addEventListener("click", ()=>{
    const msg = `AP ${apIP||"-"}  |  STA ${staIP||"-"}`; alert(msg);
  });

