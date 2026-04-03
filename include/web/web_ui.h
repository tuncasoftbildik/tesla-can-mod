#pragma once

const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#1a1a2e">
<link rel="icon" href="data:,">
<title>TeslaCAN</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,system-ui,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:16px;max-width:480px;margin:0 auto}
h1{font-size:1.3em;color:#00d4aa;margin-bottom:16px;text-align:center}
.card{background:#16213e;border-radius:12px;padding:16px;margin-bottom:12px}
.card h2{font-size:.85em;color:#666;margin-bottom:10px;text-transform:uppercase;letter-spacing:.05em}
.row{display:flex;justify-content:space-between;align-items:center;padding:8px 0}
.row+.row{border-top:1px solid #1a1a2e}
.label{color:#999;font-size:.9em}
.val{font-weight:600;font-size:1.05em}
.on{color:#00d4aa}
.off{color:#555}
.sw{position:relative;width:48px;height:26px}
.sw input{opacity:0;width:0;height:0}
.sl{position:absolute;cursor:pointer;inset:0;background:#333;border-radius:26px;transition:.3s}
.sl:before{content:"";position:absolute;height:20px;width:20px;left:3px;bottom:3px;background:#888;border-radius:50%;transition:.3s}
input:checked+.sl{background:#00d4aa}
input:checked+.sl:before{transform:translateX(22px);background:#fff}
#log{background:#0f0f23;border-radius:8px;padding:10px;height:200px;overflow-y:auto;font-family:'SF Mono',Monaco,Consolas,monospace;font-size:.75em;color:#777}
.le{padding:3px 0;border-bottom:1px solid #151528}
.ts{color:#444;margin-right:6px}
.dot{width:8px;height:8px;border-radius:50%;display:inline-block}
.dot.on{background:#00d4aa;box-shadow:0 0 6px #00d4aa}
.dot.off{background:#555}
.err{color:#ff6b6b;text-align:center;font-size:.8em;padding:8px;display:none}
</style>
</head>
<body>
<h1>TeslaCAN</h1>
<div id="connErr" class="err">Connection lost. Retrying...</div>
<div class="card"><h2>Status</h2>
<div class="row"><span class="label">FSD Active</span><span class="val" id="fsd">--</span></div>
<div class="row"><span class="label">Force FSD</span><span class="val" id="ffsd">--</span></div>
<div class="row"><span class="label">Speed Profile</span><span class="val" id="prof">--</span></div>
<div class="row"><span class="label">Speed Offset</span><span class="val" id="soff">--</span></div>
<div class="row"><span class="label">Uptime</span><span class="val" id="up">--</span></div>
</div>
<div class="card"><h2>CAN Bus</h2>
<div class="row"><span class="label">State</span><span class="val" id="canst">--</span></div>
<div class="row"><span class="label">Frames Received</span><span class="val" id="canfr">0</span></div>
<div class="row"><span class="label">Frames Modified</span><span class="val" id="canfs">0</span></div>
<div class="row"><span class="label">RX Errors</span><span class="val" id="canrx">0</span></div>
<div class="row"><span class="label">TX Errors</span><span class="val" id="cantx">0</span></div>
<div class="row"><span class="label">Bus Errors</span><span class="val" id="canbe">0</span></div>
<div class="row"><span class="label">RX Missed</span><span class="val" id="canrm">0</span></div>
</div>
<div class="card"><h2>Battery</h2>
<div class="row"><span class="label">SoC</span><span class="val" id="bsoc">--</span></div>
<div class="row"><span class="label">Voltage</span><span class="val" id="bvolt">--</span></div>
<div class="row"><span class="label">Current</span><span class="val" id="bcurr">--</span></div>
<div class="row"><span class="label">Power</span><span class="val" id="bpow">--</span></div>
<div class="row"><span class="label">Temp</span><span class="val" id="btemp">--</span></div>
<div class="row"><span class="label">Consumption</span><span class="val" id="bwhkm">--</span></div>
<div class="row"><span class="label">Precondition</span><span class="val" id="precst">--</span></div>
</div>
<div class="card"><h2>Controls</h2>
<div class="row"><span class="label">Force FSD</span>
<label class="sw"><input type="checkbox" id="tFsd" onchange="togFsd(this)"><span class="sl"></span></label></div>
<div class="row"><span class="label">Precondition</span>
<label class="sw"><input type="checkbox" id="tPrec" onchange="togPrec(this)"><span class="sl"></span></label></div>
<div class="row"><span class="label">Enable Log</span>
<label class="sw"><input type="checkbox" id="tLog" checked onchange="togLog(this)"><span class="sl"></span></label></div>
</div>
<div class="card"><h2>Log</h2><div id="log"></div></div>
<script>
var logSince=0,errCount=0;
var profNames=['Chill','Normal','Sport'];
function fmt(s){var h=Math.floor(s/3600),m=Math.floor((s%3600)/60),sec=s%60;return h+':'+(m<10?'0':'')+m+':'+(sec<10?'0':'')+sec;}
function setDot(el,on){el.textContent='';var d=document.createElement('span');d.className='dot '+(on?'on':'off');el.appendChild(d);el.appendChild(document.createTextNode(' '+(on?'Active':'Off')));el.className='val '+(on?'on':'off');}
function socColor(s){return s>60?'#00d4aa':s>30?'#f0a500':'#ff6b6b';}
async function poll(){try{var r=await fetch('/api/status?log_since='+logSince);var d=await r.json();setDot(document.getElementById('fsd'),d.fsd_enabled);setDot(document.getElementById('ffsd'),d.force_fsd);document.getElementById('prof').textContent=profNames[d.speed_profile]||('P'+d.speed_profile);document.getElementById('soff').textContent=d.speed_offset;document.getElementById('up').textContent=fmt(d.uptime_s);document.getElementById('tFsd').checked=d.force_fsd;document.getElementById('tLog').checked=d.enable_print;if(d.can){var cs=document.getElementById('canst');cs.textContent='';var cd=document.createElement('span');cd.className='dot '+(d.can.state==='RUNNING'?'on':'off');cs.appendChild(cd);cs.appendChild(document.createTextNode(' '+d.can.state));cs.className='val '+(d.can.state==='RUNNING'?'on':(d.can.state==='BUS_OFF'?'off':''));document.getElementById('canfr').textContent=d.can.frames_received;document.getElementById('canfs').textContent=d.can.frames_sent;document.getElementById('canrx').textContent=d.can.rx_errors;document.getElementById('cantx').textContent=d.can.tx_errors;document.getElementById('canbe').textContent=d.can.bus_errors;document.getElementById('canrm').textContent=d.can.rx_missed;}if(d.bat){var sc=document.getElementById('bsoc');sc.textContent=d.bat.soc.toFixed(1)+'%';sc.style.color=socColor(d.bat.soc);document.getElementById('bvolt').textContent=d.bat.voltage.toFixed(1)+'V';var cu=document.getElementById('bcurr');cu.textContent=d.bat.current.toFixed(1)+'A';cu.style.color=d.bat.current<0?'#ff6b6b':'#00d4aa';var pw=document.getElementById('bpow');pw.textContent=Math.abs(d.bat.power_kw).toFixed(1)+'kW';pw.style.color=d.bat.power_kw<0?'#ff6b6b':'#00d4aa';document.getElementById('btemp').textContent=d.bat.temp_min.toFixed(0)+'~'+d.bat.temp_max.toFixed(0)+'°C';document.getElementById('bwhkm').textContent=d.bat.wh_per_km>0?d.bat.wh_per_km.toFixed(0)+' Wh/km':'--';setDot(document.getElementById('precst'),d.bat.precond);document.getElementById('tPrec').checked=d.bat.precond_req;}if(d.logs&&d.logs.length){var el=document.getElementById('log');for(var i=0;i<d.logs.length;i++){var e=document.createElement('div');e.className='le';var ts=document.createElement('span');ts.className='ts';ts.textContent=fmt(Math.floor(d.logs[i].ts/1000));e.appendChild(ts);e.appendChild(document.createTextNode(d.logs[i].msg));el.insertBefore(e,el.firstChild);}while(el.children.length>100)el.removeChild(el.lastChild);}logSince=d.log_head;errCount=0;document.getElementById('connErr').style.display='none';}catch(e){errCount++;if(errCount>3)document.getElementById('connErr').style.display='block';}}
async function togFsd(el){if(el.checked&&!confirm('Enable Force FSD?')){el.checked=false;return;}try{await fetch('/api/force-fsd',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});}catch(e){el.checked=!el.checked;}}
async function togPrec(el){try{await fetch('/api/precond',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});}catch(e){el.checked=!el.checked;}}
async function togLog(el){try{await fetch('/api/enable-print',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});}catch(e){el.checked=!el.checked;}}
setInterval(poll,500);poll();
</script>
</body>
</html>
)rawliteral";
