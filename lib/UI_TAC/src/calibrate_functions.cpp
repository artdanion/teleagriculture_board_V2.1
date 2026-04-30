#include <calibrate_functions.h>
#include <init_Board.h>
#include <cal_values.h>
#include <file_functions.h>
#include <Adafruit_ADS1X15.h>
#include <ArduinoJson.h>

// ---- /calread  GET  ?type=ads&ch=0..3   or   ?type=adc&pin=0..2 ----

void handleCalRead()
{
    String type   = server.arg("type");
    int    ch     = server.arg("ch").toInt();
    int    pin_idx = server.arg("pin").toInt();

    float mv = NAN;

    if (type == "ads") {
        pinMode(SW_3V3, OUTPUT);
        digitalWrite(SW_3V3, HIGH);
        Wire.setPins(I2C_SDA, I2C_SCL);
        Adafruit_ADS1115 ads;
        ads.setGain(GAIN_ONE);
        if (ads.begin(0x48, &Wire)) {
            int16_t raw = ads.readADC_SingleEnded(ch);
            mv = raw * 0.125f;
        }
    } else if (type == "adc") {
        int pin = (pin_idx == 1) ? ANALOG2 : (pin_idx == 2) ? ANALOG3 : ANALOG1;
        pinMode(pin, INPUT);
        mv = (float)analogRead(pin);
    }

    StaticJsonDocument<64> doc;
    if (isnan(mv)) {
        doc["status"] = "error";
        doc["mv"]     = 0;
    } else {
        doc["status"] = "ok";
        doc["mv"]     = mv;
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

// ---- /calsave  POST  sensor=ph|ec|do|orp|soil + values ----

void handleCalSave()
{
    String sensor = server.arg("sensor");

    if (sensor == "ph") {
        float n = server.arg("neutral_mV").toFloat();
        float a = server.arg("acid_mV").toFloat();
        if (n > 0 && a > 0 && n != a) {
            cal_pH_neutral = n;
            cal_pH_acid    = a;
        }
    } else if (sensor == "ec") {
        float k = server.arg("kvalue").toFloat();
        if (k > 0) cal_EC_kvalue = k;
    } else if (sensor == "do") {
        float v = server.arg("sat_mV").toFloat();
        float t = server.arg("sat_T").toFloat();
        if (v > 0) cal_DO_sat_mV = v;
        if (t >= 0 && t <= 40) cal_DO_sat_T = t;
    } else if (sensor == "orp") {
        cal_ORP_offset = server.arg("offset_mV").toFloat();
    } else if (sensor == "soil") {
        int a = server.arg("air").toInt();
        int w = server.arg("water").toInt();
        if (a > 0 && w > 0 && a != w) {
            cal_soil_air   = a;
            cal_soil_water = w;
        }
    } else {
        server.send(400, "application/json", "{\"status\":\"unknown sensor\"}");
        return;
    }

    save_Cal();
    server.send(200, "application/json", "{\"status\":\"saved\"}");
}

// ---- /calibrate  GET  (main calibration wizard page) ----

void handleCalibrate()
{
    String page = F(R"rawhtml(<!DOCTYPE html><html>
<head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>TAC Calibration</title>
<style>
html{font-family:Helvetica;min-height:100%;background:linear-gradient(to bottom,#00CC99,#009999);color:#fff;margin:0}
body{max-width:640px;margin:0 auto;padding:16px}
h1{text-align:center;margin-bottom:4px}
.sub{text-align:center;opacity:.8;margin-bottom:20px;font-size:.9em}
.sensor-grid{display:flex;flex-wrap:wrap;gap:10px;justify-content:center;margin-bottom:20px}
.sensor-btn{background:rgba(255,255,255,.2);border:2px solid rgba(255,255,255,.45);color:#fff;
  border-radius:8px;padding:10px 18px;cursor:pointer;font-size:1em;transition:.15s;text-align:center;line-height:1.2}
.sensor-btn:hover,.sensor-btn.active{background:rgba(255,255,255,.4);border-color:#fff}
.btn-sub{font-size:.68em;opacity:.75;margin-top:3px}
.btn-sub.adv{color:#ffe066;opacity:1}
.notice{background:rgba(0,0,0,.2);border-left:3px solid #ffe066;border-radius:0 6px 6px 0;
  padding:10px 14px;font-size:.85em;line-height:1.5;margin-bottom:18px;color:rgba(255,255,255,.9)}
.wizard{display:none;background:rgba(0,0,0,.18);border-radius:10px;padding:18px;margin-bottom:14px}
.wizard.visible{display:block}
.wizard h2{margin:0 0 14px 0;font-size:1.1em;border-bottom:1px solid rgba(255,255,255,.3);padding-bottom:8px}
.step{background:rgba(255,255,255,.12);border-radius:8px;padding:13px;margin-bottom:12px}
.step h3{margin:0 0 6px 0;font-size:.95em}
.step p{margin:4px 0;opacity:.88;font-size:.88em;line-height:1.4}
.live{font-size:1.5em;font-weight:bold;margin:10px 0 6px;font-family:monospace}
.live span{background:rgba(0,0,0,.28);border-radius:4px;padding:2px 10px}
.unit{font-size:.6em;opacity:.75}
.set-btn{background:#206040;border:none;color:#fff;border-radius:6px;padding:7px 16px;
  cursor:pointer;font-size:.9em;margin-top:4px;transition:.15s}
.set-btn:hover{background:#287a50}
.set-btn.done{background:#008833}
.save-row{margin-top:6px}
.save-btn{width:100%;background:#206040;border:2px solid rgba(255,255,255,.35);
  color:#fff;border-radius:8px;padding:11px;cursor:pointer;font-size:1.05em;transition:.15s}
.save-btn:hover:not(:disabled){background:#287a50}
.save-btn:disabled{opacity:.38;cursor:not-allowed}
.status-msg{text-align:center;margin-top:8px;font-size:.9em;min-height:1.3em}
label{font-size:.88em;opacity:.9}
input[type=number]{background:rgba(0,0,0,.22);border:1px solid rgba(255,255,255,.38);
  color:#fff;border-radius:4px;padding:5px 8px;font-size:.95em;width:110px;margin-left:6px}
.back{display:block;text-align:center;color:rgba(255,255,255,.8);text-decoration:none;margin-top:14px;font-size:.9em}
.back:hover{color:#fff}
</style>
</head>
<body>
<h1>Sensor Calibration</h1>
<p class='sub'>TeleAgriCulture Board &nbsp;&mdash;&nbsp; Calibration Wizard</p>

<div class='sensor-grid'>
  <button class='sensor-btn' onclick='showWiz("ph",this)'>pH<div class='btn-sub adv'>Advanced &middot; ADS1115</div></button>
  <button class='sensor-btn' onclick='showWiz("ec",this)'>EC (Conductivity)<div class='btn-sub adv'>Advanced &middot; ADS1115</div></button>
  <button class='sensor-btn' onclick='showWiz("do",this)'>DO (Dissolved O&#8322;)<div class='btn-sub adv'>Advanced &middot; ADS1115</div></button>
  <button class='sensor-btn' onclick='showWiz("orp",this)'>ORP / Redox<div class='btn-sub adv'>Advanced &middot; ADS1115</div></button>
  <button class='sensor-btn' onclick='showWiz("soil",this)'>Soil Moisture<div class='btn-sub'>Analog pin</div></button>
</div>
<div class='notice'>&#9888; <b>ADS1115 sensors</b> (pH &middot; EC &middot; DO &middot; ORP) require the optional 4-channel ADC module connected via I2C (0x48).<br>
Web calibration wizard is functional but in active development &mdash; calibration values are saved to SPIFFS and survive reboots.</div>

<!-- pH Wizard -->
<div class='wizard' id='wiz-ph'>
  <h2>pH &mdash; 2-Point Calibration</h2>
  <div class='step'>
    <h3>Step 1 &mdash; Neutral point (pH 7.0)</h3>
    <p>Rinse the probe with distilled water, then place it in pH 7.0 buffer solution.<br>Wait until the reading is stable (approx. 30 s).</p>
    <div class='live'>Live: <span id='ph-mv1'>---</span> <span class='unit'>mV</span></div>
    <button class='set-btn' id='ph-set1' onclick='setPhNeutral()'>Set Neutral Point</button>
  </div>
  <div class='step'>
    <h3>Step 2 &mdash; Acid point (pH 4.0)</h3>
    <p>Rinse the probe, then place it in pH 4.0 buffer solution.<br>Wait until the reading is stable.</p>
    <div class='live'>Live: <span id='ph-mv2'>---</span> <span class='unit'>mV</span></div>
    <button class='set-btn' id='ph-set2' onclick='setPhAcid()'>Set Acid Point</button>
  </div>
  <div class='save-row'>
    <button class='save-btn' id='ph-save' disabled onclick='savePh()'>Save pH Calibration</button>
    <div class='status-msg' id='ph-status'></div>
  </div>
</div>

<!-- EC Wizard -->
<div class='wizard' id='wiz-ec'>
  <h2>EC &mdash; K-Value Calibration</h2>
  <div class='step'>
    <h3>Calibration with a known standard solution</h3>
    <p>Use a certified conductivity standard (e.g. 1413 &micro;S/cm = 1.413 mS/cm).<br>Rinse the probe, submerge in the solution, wait until reading is stable.</p>
    <div class='live'>Live: <span id='ec-mv'>---</span> <span class='unit'>mV</span></div>
    <p style='margin-top:10px'><label>Known EC (mS/cm):</label>
      <input type='number' id='ec-known' step='0.001' min='0.001' value='1.413'></p>
    <button class='set-btn' style='margin-top:10px' onclick='calcEC()'>Compute K Value</button>
    <p style='margin-top:8px;font-size:.88em'>Computed K: <b id='ec-kval'>–</b></p>
  </div>
  <div class='save-row'>
    <button class='save-btn' id='ec-save' disabled onclick='saveEC()'>Save EC Calibration</button>
    <div class='status-msg' id='ec-status'></div>
  </div>
</div>

<!-- DO Wizard -->
<div class='wizard' id='wiz-do'>
  <h2>DO &mdash; Air Saturation Calibration</h2>
  <div class='step'>
    <h3>Single-point calibration in air</h3>
    <p>Expose the probe to open air or air-saturated water (100% saturation reference).<br>Wait at least 60 s for the reading to stabilize.</p>
    <p><label>Current temperature (&deg;C):</label>
      <input type='number' id='do-temp' step='0.1' min='0' max='40' value='25'></p>
    <div class='live' style='margin-top:8px'>Live: <span id='do-mv'>---</span> <span class='unit'>mV</span></div>
    <button class='set-btn' id='do-set' onclick='setDO()' style='margin-top:6px'>Set Saturation Point</button>
  </div>
  <div class='save-row'>
    <button class='save-btn' id='do-save' disabled onclick='saveDO()'>Save DO Calibration</button>
    <div class='status-msg' id='do-status'></div>
  </div>
</div>

<!-- ORP Wizard -->
<div class='wizard' id='wiz-orp'>
  <h2>ORP / Redox &mdash; Offset Calibration</h2>
  <div class='step'>
    <h3>Calibration with a known ORP buffer</h3>
    <p>Use a standard ORP solution (e.g. Zobell = +228 mV at 25&deg;C, or Quinhydrone).<br>Place probe in buffer and wait until reading is stable.</p>
    <div class='live'>Raw ADC: <span id='orp-raw'>---</span> <span class='unit'>mV</span></div>
    <p style='margin-top:10px'><label>Expected ORP value (mV):</label>
      <input type='number' id='orp-exp' step='1' value='228'></p>
    <button class='set-btn' style='margin-top:10px' onclick='calcORP()'>Compute Offset</button>
    <p style='margin-top:8px;font-size:.88em'>Computed offset: <b id='orp-off'>–</b> mV</p>
  </div>
  <div class='save-row'>
    <button class='save-btn' id='orp-save' disabled onclick='saveORP()'>Save ORP Calibration</button>
    <div class='status-msg' id='orp-status'></div>
  </div>
</div>

<!-- Soil Wizard -->
<div class='wizard' id='wiz-soil'>
  <h2>Soil Moisture &mdash; 2-Point Calibration</h2>
  <div class='step'>
    <h3>Step 1 &mdash; Dry reference (in air)</h3>
    <p>Hold the probe in open air (completely dry). Wait until reading is stable.</p>
    <div class='live'>Live: <span id='soil-dry'>---</span> <span class='unit'>ADC</span></div>
    <button class='set-btn' id='soil-set1' onclick='setSoilDry()'>Set Dry Point</button>
  </div>
  <div class='step'>
    <h3>Step 2 &mdash; Wet reference (in water)</h3>
    <p>Submerge the probe in water up to the marked line. Wait until reading is stable.</p>
    <div class='live'>Live: <span id='soil-wet'>---</span> <span class='unit'>ADC</span></div>
    <button class='set-btn' id='soil-set2' onclick='setSoilWet()'>Set Wet Point</button>
  </div>
  <div class='save-row'>
    <button class='save-btn' id='soil-save' disabled onclick='saveSoil()'>Save Soil Calibration</button>
    <div class='status-msg' id='soil-status'></div>
  </div>
</div>

<a class='back' href='/'>&#8592; Back to Dashboard</a>

<script>
var activeWiz=null, liveTimer=null;
var phN=null, phA=null, ecK=null, doMV=null, orpOff=null, soilDry=null, soilWet=null;

function showWiz(name,btn){
  document.querySelectorAll('.sensor-btn').forEach(function(b){b.classList.remove('active');});
  document.querySelectorAll('.wizard').forEach(function(w){w.classList.remove('visible');});
  stopLive();
  if(activeWiz===name){activeWiz=null;return;}
  btn.classList.add('active');
  document.getElementById('wiz-'+name).classList.add('visible');
  activeWiz=name;
  fetchLive(name);
  liveTimer=setInterval(function(){fetchLive(name);},2000);
}

function stopLive(){if(liveTimer){clearInterval(liveTimer);liveTimer=null;}}

function fetchLive(name){
  var url;
  if(name==='ph')  url='/calread?type=ads&ch=3';
  else if(name==='ec')  url='/calread?type=ads&ch=2';
  else if(name==='do')  url='/calread?type=ads&ch=1';
  else if(name==='orp') url='/calread?type=ads&ch=0';
  else if(name==='soil') url='/calread?type=adc&pin=0';
  else return;
  fetch(url).then(function(r){return r.json();}).then(function(d){
    var v=d.status==='ok'?d.mv.toFixed(1):'ERR';
    if(name==='ph'){setEl('ph-mv1',v);setEl('ph-mv2',v);}
    else if(name==='ec')  setEl('ec-mv',v);
    else if(name==='do')  setEl('do-mv',v);
    else if(name==='orp') setEl('orp-raw',v);
    else if(name==='soil'){setEl('soil-dry',v);setEl('soil-wet',v);}
  }).catch(function(){});
}

function setEl(id,v){var e=document.getElementById(id);if(e)e.textContent=v;}
function getNum(id){return parseFloat(document.getElementById(id).textContent);}

// pH
function setPhNeutral(){
  var v=getNum('ph-mv1');if(isNaN(v))return;
  phN=v;mark('ph-set1','Neutral: '+v.toFixed(1)+' mV');checkPh();
}
function setPhAcid(){
  var v=getNum('ph-mv2');if(isNaN(v))return;
  phA=v;mark('ph-set2','Acid: '+v.toFixed(1)+' mV');checkPh();
}
function checkPh(){if(phN!==null&&phA!==null)enable('ph-save');}
function savePh(){
  post('sensor=ph&neutral_mV='+phN+'&acid_mV='+phA,'ph-status');
}

// EC
function calcEC(){
  var mv=getNum('ec-mv');
  var known=parseFloat(document.getElementById('ec-known').value);
  if(isNaN(mv)||mv<=0||isNaN(known)||known<=0)return;
  var rawEC=mv/820.0/200.0;
  if(rawEC<=0)return;
  ecK=known/rawEC;
  setEl('ec-kval',ecK.toFixed(4));
  enable('ec-save');
}
function saveEC(){post('sensor=ec&kvalue='+ecK,'ec-status');}

// DO
function setDO(){
  var v=getNum('do-mv');if(isNaN(v))return;
  doMV=v;
  var t=parseFloat(document.getElementById('do-temp').value);
  mark('do-set','Set: '+v.toFixed(1)+' mV @ '+t.toFixed(1)+'°C');
  enable('do-save');
}
function saveDO(){
  var t=parseFloat(document.getElementById('do-temp').value);
  post('sensor=do&sat_mV='+doMV+'&sat_T='+t,'do-status');
}

// ORP
function calcORP(){
  var raw=getNum('orp-raw');
  var exp=parseFloat(document.getElementById('orp-exp').value);
  if(isNaN(raw)||isNaN(exp))return;
  orpOff=exp-raw;
  setEl('orp-off',orpOff.toFixed(1));
  enable('orp-save');
}
function saveORP(){post('sensor=orp&offset_mV='+orpOff,'orp-status');}

// Soil
function setSoilDry(){
  var v=getNum('soil-dry');if(isNaN(v))return;
  soilDry=Math.round(v);mark('soil-set1','Dry: '+soilDry);checkSoil();
}
function setSoilWet(){
  var v=getNum('soil-wet');if(isNaN(v))return;
  soilWet=Math.round(v);mark('soil-set2','Wet: '+soilWet);checkSoil();
}
function checkSoil(){if(soilDry!==null&&soilWet!==null)enable('soil-save');}
function saveSoil(){post('sensor=soil&air='+soilDry+'&water='+soilWet,'soil-status');}

// helpers
function mark(id,txt){var e=document.getElementById(id);if(!e)return;e.textContent=txt;e.classList.add('done');}
function enable(id){var e=document.getElementById(id);if(e)e.disabled=false;}
function post(body,statusId){
  fetch('/calsave',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
  .then(function(r){return r.json();}).then(function(d){
    setEl(statusId,d.status==='saved'?'Saved successfully!':'Error: '+d.status);
  }).catch(function(){setEl(statusId,'Network error.');});
}
</script>
</body></html>)rawhtml");

    server.send(200, "text/html", page);
}
