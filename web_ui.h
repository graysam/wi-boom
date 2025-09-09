#pragma once

// Minimal UI served from PROGMEM. For a richer UI, place files in /data and use PlatformIO uploadfs.
static const char INDEX_HTML[] PROGMEM = R"html(
<!doctype html>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>HV Trigger</title>
<style>
  body{font-family:system-ui,Segoe UI,Arial;margin:24px;}
  .row{margin:8px 0}
  label{display:inline-block;width:90px}
  input[type=number]{width:70px}
  button{margin-right:8px}
</style>
<h2>HV Trigger Async</h2>
<div class="row">
  <button id="arm">Arm</button>
  <button id="disarm">Disarm</button>
  <button id="fire" disabled>Fire</button>
  <span id="armedLbl" style="margin-left:8px;color:#a00">disarmed</span>
  <span id="connLbl" style="margin-left:8px;color:#555">connecting…</span>
  <span id="status"></span>
</div>
<div class="row">
  <label>Mode</label>
  <select id="mode"><option value="single">single</option><option value="buzz">buzz</option></select>
  <label>Width</label><input id="width" type="number" min="5" max="100" value="10"/>
  <label>Spacing</label><input id="spacing" type="number" min="10" max="100" value="20"/>
  <label>Repeat</label><input id="repeat" type="number" min="1" max="4" value="1"/>
  <button id="apply">Apply</button>
  <span id="status"></span>
  <div id="telemetry" style="margin-top:8px;color:#555"></div>
</div>
<script>
const ws = new WebSocket(`ws://${location.host}/ws`);
const el = s=>document.querySelector(s);
const send = o=>ws.readyState===1 && ws.send(JSON.stringify(o));
ws.onopen = ()=> el('#connLbl').textContent='connected';
ws.onclose= ()=> el('#connLbl').textContent='disconnected';
ws.onmessage = (ev)=> {
  try { const j=JSON.parse(ev.data); if(j.type==='state'){ render(j); } } catch(e){}
};
function render(s){
  el('#telemetry').textContent = `armed=${s.armed} firing=${s.pulseActive} wifiClients=${s.wifiClients} mode=${s.cfg.mode} width=${s.cfg.width} spacing=${s.cfg.spacing} repeat=${s.cfg.repeat}`;
  el('#mode').value = s.cfg.mode;
  el('#width').value = s.cfg.width;
  el('#spacing').value = s.cfg.spacing;
  el('#repeat').value = s.cfg.repeat;
  el('#armedLbl').textContent = s.armed ? 'armed' : 'disarmed';
  el('#armedLbl').style.color = s.armed ? '#090' : '#a00';
  el('#fire').disabled = !s.armed;
}
el('#arm').onclick = ()=> send({cmd:'arm', on:true});
el('#disarm').onclick = ()=> send({cmd:'arm', on:false});
el('#fire').onclick = ()=> send({cmd:'fire'});
el('#apply').onclick = ()=> send({cmd:'cfg', mode:el('#mode').value, width:+el('#width').value, spacing:+el('#spacing').value, repeat:+el('#repeat').value});
</script>
)html";

