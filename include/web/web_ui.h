#pragma once

const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="theme-color" content="#0a0a1a">
<link rel="icon" href="data:,">
<title>TeslaCAN</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap');
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#0a0a1a;--card:#111827;--card2:#1a1f35;--accent:#00d4aa;--accent2:#00b894;--red:#ff6b6b;--yellow:#ffd93d;--blue:#4dabf7;--border:#1e293b;--text:#e2e8f0;--text2:#94a3b8;--text3:#475569}
body{font-family:'Inter',-apple-system,system-ui,sans-serif;background:var(--bg);color:var(--text);padding:0;margin:0;min-height:100vh}
.wrap{max-width:480px;margin:0 auto;padding:16px 16px 32px}

/* Header */
.header{text-align:center;padding:20px 0 16px;position:relative}
.header h1{font-size:1.5em;font-weight:700;background:linear-gradient(135deg,var(--accent),var(--blue));-webkit-background-clip:text;-webkit-text-fill-color:transparent;letter-spacing:-.02em}
.header .sub{font-size:.7em;color:var(--text3);margin-top:2px;letter-spacing:.1em;text-transform:uppercase}
.conn{position:absolute;right:0;top:24px;width:8px;height:8px;border-radius:50%;background:var(--accent);box-shadow:0 0 8px var(--accent);transition:.3s}
.conn.off{background:var(--red);box-shadow:0 0 8px var(--red)}

/* Hero Battery */
.hero{background:linear-gradient(145deg,#0f1729,#162033);border-radius:20px;padding:24px;margin-bottom:16px;position:relative;overflow:hidden;border:1px solid var(--border)}
.hero::before{content:'';position:absolute;top:-50%;right:-50%;width:100%;height:100%;background:radial-gradient(circle,rgba(0,212,170,.06) 0%,transparent 70%);pointer-events:none}
.soc-ring{width:120px;height:120px;margin:0 auto 16px;position:relative}
.soc-ring svg{transform:rotate(-90deg)}
.soc-ring .track{fill:none;stroke:#1e293b;stroke-width:8}
.soc-ring .bar{fill:none;stroke:var(--accent);stroke-width:8;stroke-linecap:round;transition:stroke-dashoffset .8s ease,stroke .5s}
.soc-val{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center}
.soc-num{font-size:2em;font-weight:700;line-height:1}
.soc-lbl{font-size:.65em;color:var(--text3);margin-top:2px}
.hero-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;text-align:center}
.hero-grid .hv{font-size:1.15em;font-weight:600}
.hero-grid .hl{font-size:.65em;color:var(--text3);margin-top:2px}

/* Cards */
.card{background:var(--card);border-radius:16px;padding:16px;margin-bottom:12px;border:1px solid var(--border)}
.card-head{display:flex;align-items:center;gap:8px;margin-bottom:12px}
.card-head .icon{width:28px;height:28px;border-radius:8px;display:flex;align-items:center;justify-content:center;font-size:.85em}
.card-head h2{font-size:.8em;font-weight:600;color:var(--text2);text-transform:uppercase;letter-spacing:.06em}
.ic-fsd{background:rgba(0,212,170,.15);color:var(--accent)}
.ic-can{background:rgba(77,171,247,.15);color:var(--blue)}
.ic-ctrl{background:rgba(255,217,61,.15);color:var(--yellow)}
.ic-log{background:rgba(148,163,184,.15);color:var(--text2)}

.row{display:flex;justify-content:space-between;align-items:center;padding:10px 0}
.row+.row{border-top:1px solid rgba(255,255,255,.04)}
.label{color:var(--text2);font-size:.85em}
.val{font-weight:600;font-size:.95em;font-variant-numeric:tabular-nums}

/* Status Pill */
.pill{display:inline-flex;align-items:center;gap:5px;padding:3px 10px;border-radius:20px;font-size:.8em;font-weight:600}
.pill.on{background:rgba(0,212,170,.15);color:var(--accent)}
.pill.off{background:rgba(71,85,105,.2);color:var(--text3)}
.pill.warn{background:rgba(255,107,107,.15);color:var(--red)}
.pill .dot{width:6px;height:6px;border-radius:50%;flex-shrink:0}
.pill.on .dot{background:var(--accent);box-shadow:0 0 6px var(--accent)}
.pill.off .dot{background:var(--text3)}
.pill.warn .dot{background:var(--red);box-shadow:0 0 6px var(--red)}

/* Toggle */
.sw{position:relative;width:44px;height:24px;flex-shrink:0}
.sw input{opacity:0;width:0;height:0}
.sl{position:absolute;cursor:pointer;inset:0;background:#2a2a3e;border-radius:24px;transition:.3s}
.sl:before{content:"";position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#555;border-radius:50%;transition:.3s}
input:checked+.sl{background:var(--accent)}
input:checked+.sl:before{transform:translateX(20px);background:#fff}

/* Mini stat grid for CAN */
.stat-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
.stat-box{background:var(--card2);border-radius:10px;padding:10px 12px}
.stat-box .sv{font-size:1.1em;font-weight:600;font-variant-numeric:tabular-nums}
.stat-box .sl2{font-size:.65em;color:var(--text3);margin-top:2px}

/* Log */
#log{background:#080816;border-radius:10px;padding:10px;height:180px;overflow-y:auto;font-family:'SF Mono',Monaco,Consolas,monospace;font-size:.72em;color:var(--text3)}
#log::-webkit-scrollbar{width:4px}
#log::-webkit-scrollbar-thumb{background:#1e293b;border-radius:4px}
.le{padding:4px 0;border-bottom:1px solid rgba(255,255,255,.03)}
.ts{color:var(--text3);opacity:.5;margin-right:8px}

/* Error banner */
.err{color:var(--red);text-align:center;font-size:.8em;padding:10px;display:none;background:rgba(255,107,107,.08);border-radius:10px;margin-bottom:12px;border:1px solid rgba(255,107,107,.15)}

/* Footer */
.foot{text-align:center;padding:16px 0 0;font-size:.65em;color:var(--text3)}
.foot a{color:var(--accent);text-decoration:none}

/* Animations */
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
.loading{animation:pulse 1.5s infinite}
</style>
</head>
<body>
<div class="wrap">

<!-- Header -->
<div class="header">
  <h1>TeslaCAN</h1>
  <div class="sub">ESP32-C6 CAN Veri Yolu Kontrolcüsü</div>
  <div class="conn" id="connDot"></div>
</div>
<div id="connErr" class="err">Bağlantı koptu. Yeniden deneniyor...</div>

<!-- Battery Hero -->
<div class="hero">
  <div class="soc-ring">
    <svg viewBox="0 0 120 120" width="120" height="120">
      <circle class="track" cx="60" cy="60" r="52"/>
      <circle class="bar" id="socBar" cx="60" cy="60" r="52" stroke-dasharray="326.73" stroke-dashoffset="326.73"/>
    </svg>
    <div class="soc-val">
      <span class="soc-num" id="bsoc">--</span>
      <span class="soc-lbl">BATARYA</span>
    </div>
  </div>
  <div class="hero-grid">
    <div><div class="hv" id="bpow">--</div><div class="hl">Güç</div></div>
    <div><div class="hv" id="bvolt">--</div><div class="hl">Gerilim</div></div>
    <div><div class="hv" id="btemp">--</div><div class="hl">Sıcaklık</div></div>
    <div><div class="hv" id="bcurr">--</div><div class="hl">Akım</div></div>
    <div><div class="hv" id="bwhkm">--</div><div class="hl">Wh/km</div></div>
    <div><div class="hv" id="precst">--</div><div class="hl">Ön Koşul.</div></div>
  </div>
</div>

<!-- FSD Status -->
<div class="card">
  <div class="card-head"><div class="icon ic-fsd">F</div><h2>Sürüş</h2></div>
  <div class="row"><span class="label">FSD</span><span class="val" id="fsd">--</span></div>
  <div class="row"><span class="label">FSD Zorla</span><span class="val" id="ffsd">--</span></div>
  <div class="row"><span class="label">Hız Profili</span><span class="val" id="prof">--</span></div>
  <div class="row"><span class="label">Hız Ofseti</span><span class="val" id="soff">--</span></div>
  <div class="row"><span class="label">Çalışma Süresi</span><span class="val" id="up">--</span></div>
</div>

<!-- CAN Bus -->
<div class="card">
  <div class="card-head"><div class="icon ic-can">C</div><h2>CAN Veri Yolu</h2></div>
  <div class="row"><span class="label">Durum</span><span class="val" id="canst">--</span></div>
  <div class="stat-grid">
    <div class="stat-box"><div class="sv" id="canfr">0</div><div class="sl2">Alınan Çerçeve</div></div>
    <div class="stat-box"><div class="sv" id="canfs">0</div><div class="sl2">Gönderilen Çerçeve</div></div>
    <div class="stat-box"><div class="sv" id="canrx">0</div><div class="sl2">RX Hatası</div></div>
    <div class="stat-box"><div class="sv" id="cantx">0</div><div class="sl2">TX Hatası</div></div>
    <div class="stat-box"><div class="sv" id="canbe">0</div><div class="sl2">Bus Hatası</div></div>
    <div class="stat-box"><div class="sv" id="canrm">0</div><div class="sl2">Kaçan Çerçeve</div></div>
  </div>
</div>

<!-- Controls -->
<div class="card">
  <div class="card-head"><div class="icon ic-ctrl">S</div><h2>Kontroller</h2></div>
  <div class="row"><span class="label">FSD Zorla</span>
    <label class="sw"><input type="checkbox" id="tFsd" onchange="togFsd(this)"><span class="sl"></span></label></div>
  <div class="row"><span class="label">Batarya Ön Koşullandırma</span>
    <label class="sw"><input type="checkbox" id="tPrec" onchange="togPrec(this)"><span class="sl"></span></label></div>
  <div class="row"><span class="label">Kayıt Tutma</span>
    <label class="sw"><input type="checkbox" id="tLog" checked onchange="togLog(this)"><span class="sl"></span></label></div>
  <div class="row"><span class="label">Acil Durum Algılama (bit 59)</span>
    <label class="sw"><input type="checkbox" id="tEm" onchange="togEm(this)"><span class="sl"></span></label></div>
  <div class="row"><span class="label">ISA Hız Limiti Bypass</span>
    <label class="sw"><input type="checkbox" id="tIsaOvr" onchange="togIsaOvr(this)"><span class="sl"></span></label></div>
  <div class="row"><span class="label">ISA Uyarı Sesi Susturma</span>
    <label class="sw"><input type="checkbox" id="tIsaSup" onchange="togIsaSup(this)"><span class="sl"></span></label></div>
  <div class="row"><span class="label">ISA Hız Çarpanı</span>
    <input type="range" id="tIsaMul" min="0" max="7" step="1" oninput="document.getElementById('tIsaMulV').textContent=this.value" onchange="setIsaMul(this)" style="width:140px">
    <span class="val" id="tIsaMulV">7</span></div>
  <div class="row"><span class="label">BMS Isıtmaya İzin Veriyor</span><span class="val" id="pAllow">--</span></div>
</div>

<!-- Log -->
<div class="card">
  <div class="card-head"><div class="icon ic-log">L</div><h2>Olay Kaydı</h2></div>
  <div id="log"></div>
</div>

<div class="foot">TeslaCAN v1.0 &middot; <a href="https://github.com/tuncasoftbildik/tesla-can-mod" target="_blank">GitHub</a></div>

</div>
<script>
var logSince=0,errCount=0;
var profNames=['Sakin','Normal','Spor','P3','P4'];
var CIRC=326.73;

function fmt(s){var h=Math.floor(s/3600),m=Math.floor((s%3600)/60),sec=s%60;return h+':'+(m<10?'0':'')+m+':'+(sec<10?'0':'')+sec;}

function setPill(el,on,txt){el.innerHTML='<span class="dot"></span>'+txt;el.className='pill '+(on?'on':'off');}

function socColor(s){return s>60?'var(--accent)':s>30?'var(--yellow)':'var(--red)';}

function setSocRing(pct){
  var bar=document.getElementById('socBar');
  var offset=CIRC-(CIRC*Math.min(pct,100)/100);
  bar.style.strokeDashoffset=offset;
  bar.style.stroke=pct>60?'var(--accent)':pct>30?'var(--yellow)':'var(--red)';
}

async function poll(){
  try{
    var r=await fetch('/api/status?log_since='+logSince);
    var d=await r.json();

    // FSD
    setPill(document.getElementById('fsd'),d.fsd_enabled,d.fsd_enabled?'Aktif':'Kapalı');
    setPill(document.getElementById('ffsd'),d.force_fsd,d.force_fsd?'Zorlanıyor':'Kapalı');
    document.getElementById('prof').textContent=profNames[d.speed_profile]||('P'+d.speed_profile);
    document.getElementById('soff').textContent=(d.speed_offset>0?'+':'')+d.speed_offset+' km/h';
    document.getElementById('up').textContent=fmt(d.uptime_s);
    document.getElementById('tFsd').checked=d.force_fsd;
    document.getElementById('tLog').checked=d.enable_print;

    // CAN
    if(d.can){
      var cs=document.getElementById('canst');
      var isRun=d.can.state==='RUNNING';
      setPill(cs,isRun,d.can.state);
      if(d.can.state==='BUS_OFF')cs.className='pill warn';
      document.getElementById('canfr').textContent=d.can.frames_received.toLocaleString();
      document.getElementById('canfs').textContent=d.can.frames_sent.toLocaleString();
      document.getElementById('canrx').textContent=d.can.rx_errors;
      document.getElementById('cantx').textContent=d.can.tx_errors;
      document.getElementById('canbe').textContent=d.can.bus_errors;
      document.getElementById('canrm').textContent=d.can.rx_missed;
    }

    // Battery
    if(d.bat){
      var soc=d.bat.soc;
      var sn=document.getElementById('bsoc');
      sn.textContent=soc.toFixed(0)+'%';
      sn.style.color=socColor(soc);
      setSocRing(soc);

      var pw=document.getElementById('bpow');
      var pkw=d.bat.power_kw;
      pw.textContent=Math.abs(pkw).toFixed(1)+'kW';
      pw.style.color=pkw<0?'var(--red)':'var(--accent)';

      document.getElementById('bvolt').textContent=d.bat.voltage.toFixed(0)+'V';

      var cu=document.getElementById('bcurr');
      cu.textContent=d.bat.current.toFixed(1)+'A';
      cu.style.color=d.bat.current<0?'var(--red)':'var(--accent)';

      document.getElementById('btemp').textContent=d.bat.temp_min.toFixed(0)+'-'+d.bat.temp_max.toFixed(0)+'°C';
      document.getElementById('bwhkm').textContent=d.bat.wh_per_km>0?d.bat.wh_per_km.toFixed(0):'--';

      var pr=document.getElementById('precst');
      if(d.bat.precond){pr.textContent='Isıtılıyor';pr.style.color='var(--yellow)';}
      else{pr.textContent='Kapalı';pr.style.color='var(--text3)';}
      document.getElementById('tPrec').checked=d.bat.precond_req;
      var pa=document.getElementById('pAllow');
      if(d.bat.precond_allowed){pa.textContent='Evet';pa.style.color='var(--accent)';}
      else{pa.textContent='Hayır';pa.style.color='var(--text3)';}
    }

    // Toggles sync
    document.getElementById('tEm').checked=!!d.em_detect;
    document.getElementById('tIsaOvr').checked=!!d.isa_ovr;
    document.getElementById('tIsaSup').checked=!!d.isa_sup;
    if(typeof d.isa_mul!=='undefined'){
      document.getElementById('tIsaMul').value=d.isa_mul;
      document.getElementById('tIsaMulV').textContent=d.isa_mul;
    }

    // Logs
    if(d.logs&&d.logs.length){
      var el=document.getElementById('log');
      for(var i=0;i<d.logs.length;i++){
        var e=document.createElement('div');e.className='le';
        var ts=document.createElement('span');ts.className='ts';
        ts.textContent=fmt(Math.floor(d.logs[i].ts/1000));
        e.appendChild(ts);
        e.appendChild(document.createTextNode(d.logs[i].msg));
        el.insertBefore(e,el.firstChild);
      }
      while(el.children.length>100)el.removeChild(el.lastChild);
    }
    logSince=d.log_head;
    errCount=0;
    document.getElementById('connErr').style.display='none';
    document.getElementById('connDot').className='conn';
  }catch(e){
    errCount++;
    if(errCount>3){
      document.getElementById('connErr').style.display='block';
      document.getElementById('connDot').className='conn off';
    }
  }
}

async function togFsd(el){
  if(el.checked&&!confirm('FSD Zorla etkinleştirilsin mi?\nBu, güvenlik açısından kritik CAN mesajlarını değiştirir.')){el.checked=false;return;}
  try{await fetch('/api/force-fsd',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});}
  catch(e){el.checked=!el.checked;}
}
async function togPrec(el){
  try{await fetch('/api/precond',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});}
  catch(e){el.checked=!el.checked;}
}
async function togLog(el){
  try{await fetch('/api/enable-print',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});}
  catch(e){el.checked=!el.checked;}
}

async function togEm(el){try{await fetch('/api/em-detect',{method:'POST',body:JSON.stringify({enabled:el.checked})});}catch(e){el.checked=!el.checked;}}
async function togIsaOvr(el){try{await fetch('/api/isa-override',{method:'POST',body:JSON.stringify({enabled:el.checked})});}catch(e){el.checked=!el.checked;}}
async function togIsaSup(el){try{await fetch('/api/isa-suppress',{method:'POST',body:JSON.stringify({enabled:el.checked})});}catch(e){el.checked=!el.checked;}}
async function setIsaMul(el){try{await fetch('/api/isa-mul',{method:'POST',body:String(el.value)});}catch(e){}}

setInterval(poll,500);poll();
</script>
</body>
</html>
)rawliteral";
