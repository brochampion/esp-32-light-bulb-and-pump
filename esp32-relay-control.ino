#include <WiFi.h>
#include <WebServer.h>

// ---------- SETTINGS ----------
const char* AP_SSID = "smart bulb";   // Wi-Fi the ESP32 creates
const char* AP_PASS = "12345678";     // at least 8 characters

const bool ACTIVE_LOW = true;         // most relay modules switch ON with LOW. If yours is reversed, set false
const bool SELF_TEST  = true;         // clicks each relay once at boot so you can hear/see it. Set false to skip

const int N = 3;
const int   pins[N]  = {13, 25, 27};  // change these to match the pins your IN1, IN2, IN3 wires are on
const char* names[N] = {"Bulb 1", "Bulb 2", "Pump"};
const char* types[N] = {"bulb", "bulb", "pump"};   // "bulb" or "pump" decides the animation
// -------------------------------

WebServer server(80);
bool state[N];
unsigned long offAt[N];
int lastStations = -1;

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Control</title>
<style>
:root{--bg:#0b1020;--card:#151c33;--txt:#e8ecff;--mut:#8791b8;--on:#ffd54a;--water:#3ec6ff}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;font-family:system-ui,sans-serif;background:radial-gradient(circle at 50% 0,#1a2350,var(--bg) 60%);color:var(--txt);min-height:100vh;padding:18px}
h1{text-align:center;margin:6px 0 2px;font-size:1.5rem}
.sub{text-align:center;color:var(--mut);font-size:.85rem;margin-bottom:16px}
.bar{display:flex;gap:8px;max-width:480px;margin:0 auto 16px}
.bar button,.bar select{flex:1;padding:12px;border-radius:12px;border:0;font-size:.95rem;background:#222c52;color:var(--txt)}
.bar button.all{background:linear-gradient(135deg,#ffb300,#ff6d00);color:#111;font-weight:700}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:14px;max-width:480px;margin:0 auto}
.card{background:var(--card);border-radius:20px;padding:16px 10px 14px;text-align:center;position:relative;overflow:hidden;transition:.4s;border:2px solid transparent;cursor:pointer;user-select:none}
.card.on{border-color:var(--on);box-shadow:0 0 30px rgba(255,213,74,.35)}
.card.pump.on{border-color:var(--water);box-shadow:0 0 30px rgba(62,198,255,.35)}
.card.pop{animation:pop .6s}
@keyframes pop{0%{transform:scale(1)}40%{transform:scale(1.08)}100%{transform:scale(1)}}
.ripple{position:absolute;left:50%;top:40%;width:20px;height:20px;border-radius:50%;background:rgba(255,213,74,.5);transform:translate(-50%,-50%) scale(0);animation:rip .8s ease-out forwards;pointer-events:none}
.pump .ripple{background:rgba(62,198,255,.5)}
@keyframes rip{to{transform:translate(-50%,-50%) scale(14);opacity:0}}
.ic{width:88px;height:88px}
.glass{fill:#2a3358;stroke:#4b5686;stroke-width:2;transition:.5s}
.base{fill:#4b5686;transition:.5s}
.rays{opacity:0;stroke:var(--on);stroke-width:3;stroke-linecap:round;transform-origin:32px 24px;transition:.4s}
.on .glass{fill:var(--on);stroke:#fff3b0;filter:drop-shadow(0 0 10px var(--on))}
.on .rays{opacity:1;animation:spin 6s linear infinite,flick 1.2s ease-in-out infinite}
@keyframes spin{to{transform:rotate(360deg)}}
@keyframes flick{50%{stroke-width:4.5}}
.pump .glass{fill:#2a3358}
.pump.on .glass{fill:var(--water);stroke:#b8ecff;filter:drop-shadow(0 0 10px var(--water))}
.fan{transform-origin:21px 32px}
.on .fan{animation:spin .6s linear infinite}
.pipe{fill:#2a3358;transition:.5s}
.on .pipe{fill:#1b6f94}
.drop{fill:var(--water);opacity:0}
.on .drop{animation:flow 1s linear infinite}
.on .d2{animation-delay:.33s}.on .d3{animation-delay:.66s}
@keyframes flow{0%{transform:translateX(0);opacity:0}20%{opacity:1}100%{transform:translateX(22px);opacity:0}}
.name{font-weight:700;margin-top:4px}
.st{font-size:.8rem;color:var(--mut);height:18px}
.on .st{color:var(--txt)}
.pill{display:inline-block;margin-top:6px;width:54px;height:28px;border-radius:20px;background:#2a3358;position:relative;transition:.3s}
.pill:after{content:"";position:absolute;top:3px;left:3px;width:22px;height:22px;border-radius:50%;background:#fff;transition:.3s}
.on .pill{background:#ffb300}.pump.on .pill{background:#1aa8e0}
.on .pill:after{left:29px}
#msg{text-align:center;color:var(--mut);font-size:.8rem;margin-top:16px}
@media(min-width:900px){body{padding:40px}h1{font-size:2.2rem}.bar,.grid{max-width:1100px}.grid{grid-template-columns:repeat(3,1fr);gap:22px}.card{padding:28px 14px 22px}.card:hover{transform:translateY(-4px)}.ic{width:120px;height:120px}.name{font-size:1.2rem}.bar button,.bar select{padding:16px;font-size:1rem;cursor:pointer}}
</style></head><body>
<h1>Smart Control</h1>
<div class="sub">Tap a card to switch it</div>
<div class="bar">
  <button class="all" onclick="all_(1)">All ON</button>
  <button onclick="all_(0)">All OFF</button>
  <select id="tm" title="Auto-off timer">
    <option value="0">Auto-off: never</option>
    <option value="10">Auto-off: 10s</option>
    <option value="30">Auto-off: 30s</option>
    <option value="60">Auto-off: 1 min</option>
    <option value="300">Auto-off: 5 min</option>
  </select>
</div>
<div class="grid" id="grid"></div>
<div id="msg">connecting...</div>
<script>
let built=false,prev=[];
const bulb=`<svg viewBox="0 0 64 64" class="ic"><g class="rays"><line x1="32" y1="2" x2="32" y2="-2"/><line x1="8" y1="24" x2="2" y2="24"/><line x1="56" y1="24" x2="62" y2="24"/><line x1="14" y1="6" x2="10" y2="2"/><line x1="50" y1="6" x2="54" y2="2"/></g><path class="glass" d="M32 8a16 16 0 0 0-9 29v7h18v-7a16 16 0 0 0-9-29z"/><rect class="base" x="24" y="46" width="16" height="4" rx="2"/><rect class="base" x="27" y="52" width="10" height="4" rx="2"/></svg>`;
const pump=`<svg viewBox="0 0 64 64" class="ic"><rect class="pipe" x="36" y="28" width="26" height="8"/><circle class="drop" cx="40" cy="32" r="2.5"/><circle class="drop d2" cx="40" cy="32" r="2.5"/><circle class="drop d3" cx="40" cy="32" r="2.5"/><circle class="glass" cx="21" cy="32" r="17"/><g class="fan"><path d="M21 32L21 19L26 24ZM21 32L34 32L29 37ZM21 32L21 45L16 40ZM21 32L8 32L13 27Z" fill="#0b1020"/></g></svg>`;
function build(d){
  const g=document.getElementById('grid');g.innerHTML='';
  d.forEach((c,i)=>{
    const el=document.createElement('div');
    el.className='card '+c.t;el.id='c'+i;
    el.innerHTML=(c.t=='pump'?pump:bulb)+`<div class="name">${c.n}</div><div class="st" id="s${i}"></div><div class="pill"></div>`;
    el.onclick=()=>tog(i);
    g.appendChild(el);
  });built=true;
}
function upd(d){
  if(!built)build(d);
  d.forEach((c,i)=>{
    const el=document.getElementById('c'+i);
    const was=prev[i];
    el.classList.toggle('on',c.on);
    if(was!==undefined&&was!==c.on&&c.on){
      el.classList.remove('pop');void el.offsetWidth;el.classList.add('pop');
      const r=document.createElement('div');r.className='ripple';el.appendChild(r);
      setTimeout(()=>r.remove(),900);
    }
    document.getElementById('s'+i).textContent=c.on?(c.left>0?'ON - off in '+c.left+'s':(c.t=='pump'?'RUNNING':'ON')):'OFF';
    prev[i]=c.on;
  });
}
async function api(u){
  try{const r=await fetch(u);const j=await r.json();upd(j.ch);document.getElementById('msg').textContent='connected';}
  catch(e){document.getElementById('msg').textContent='connection lost...';}
}
function tog(i){
  const on=!prev[i];
  upd_local(i,on);
  api(`/api/set?ch=${i}&on=${on?1:0}&timer=${document.getElementById('tm').value}`);
}
function upd_local(i,on){
  const el=document.getElementById('c'+i);
  el.classList.toggle('on',on);
  if(on){el.classList.remove('pop');void el.offsetWidth;el.classList.add('pop');
    const r=document.createElement('div');r.className='ripple';el.appendChild(r);setTimeout(()=>r.remove(),900);}
  prev[i]=on;
}
function all_(on){api(`/api/all?on=${on}&timer=${document.getElementById('tm').value}`);}
document.addEventListener('keydown',e=>{
  if(e.target.tagName=='SELECT')return;
  const k=parseInt(e.key);
  if(k>=1&&k<=prev.length)tog(k-1);
  if(e.key=='a'||e.key=='A')all_(1);
  if(e.key=='o'||e.key=='O')all_(0);
});
api('/api/status');
setInterval(()=>api('/api/status'),1500);
</script></body></html>
)rawliteral";

// ON  = drive the pin to the active level.
// OFF = for active-low modules, RELEASE the pin (input, floating) so the module's own
//       pull-up turns the relay fully off. A 3.3V HIGH from the ESP32 can leave a
//       5V relay module half on.
void setRelay(int i, bool on) {
  state[i] = on;
  if (ACTIVE_LOW) {
    if (on) { pinMode(pins[i], OUTPUT); digitalWrite(pins[i], LOW); }
    else    { pinMode(pins[i], INPUT); }
  } else {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], on ? HIGH : LOW);
  }
  Serial.printf("[RELAY] %-6s GPIO %2d -> %-3s | pin reads %s\n",
                names[i], pins[i], on ? "ON" : "OFF",
                digitalRead(pins[i]) == HIGH ? "HIGH" : "LOW");
}

void sendStatus() {
  String j = "{\"ch\":[";
  for (int i = 0; i < N; i++) {
    long left = 0;
    if (state[i] && offAt[i] > 0) left = (long)((offAt[i] - millis()) / 1000) + 1;
    if (i) j += ",";
    j += "{\"n\":\"" + String(names[i]) + "\",\"t\":\"" + String(types[i]) + "\",\"on\":" + (state[i] ? "true" : "false") + ",\"left\":" + String(left) + "}";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

void handleSet() {
  int ch = server.arg("ch").toInt();
  bool on = server.arg("on") == "1";
  unsigned long t = server.arg("timer").toInt();
  Serial.printf("[HTTP] /api/set ch=%d on=%d timer=%lus\n", ch, on, t);
  if (ch >= 0 && ch < N) {
    setRelay(ch, on);
    offAt[ch] = (on && t > 0) ? millis() + t * 1000UL : 0;
  } else {
    Serial.printf("[HTTP] ERROR: channel %d is out of range (0-%d)\n", ch, N - 1);
  }
  sendStatus();
}

void handleAll() {
  bool on = server.arg("on") == "1";
  unsigned long t = server.arg("timer").toInt();
  Serial.printf("[HTTP] /api/all on=%d timer=%lus\n", on, t);
  for (int i = 0; i < N; i++) {
    setRelay(i, on);
    offAt[i] = (on && t > 0) ? millis() + t * 1000UL : 0;
  }
  sendStatus();
}

void selfTest() {
  Serial.println("[TEST] Self-test: each relay will switch ON for 1s then OFF");
  for (int i = 0; i < N; i++) {
    Serial.printf("[TEST] Testing %s on GPIO %d ...\n", names[i], pins[i]);
    setRelay(i, true);
    delay(1000);
    setRelay(i, false);
    delay(500);
  }
  Serial.println("[TEST] Self-test finished");
}

// Type in the Serial Monitor: 1, 2, 3 = toggle that channel | a = all ON | o = all OFF | s = status
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r' || c == ' ') continue;
    if (c >= '1' && c <= '0' + N) {
      int i = c - '1';
      Serial.printf("[SERIAL] toggle %s\n", names[i]);
      setRelay(i, !state[i]);
      offAt[i] = 0;
    } else if (c == 'a' || c == 'A') {
      Serial.println("[SERIAL] all ON");
      for (int i = 0; i < N; i++) { setRelay(i, true); offAt[i] = 0; }
    } else if (c == 'o' || c == 'O') {
      Serial.println("[SERIAL] all OFF");
      for (int i = 0; i < N; i++) { setRelay(i, false); offAt[i] = 0; }
    } else if (c == 's' || c == 'S') {
      for (int i = 0; i < N; i++)
        Serial.printf("[STATUS] %-6s GPIO %2d state=%s pin=%s\n", names[i], pins[i],
                      state[i] ? "ON" : "OFF", digitalRead(pins[i]) ? "HIGH" : "LOW");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("========== Smart Control booting ==========");
  Serial.printf("ACTIVE_LOW = %s (ON = %s)\n", ACTIVE_LOW ? "true" : "false", ACTIVE_LOW ? "LOW" : "HIGH");

  for (int i = 0; i < N; i++) {
    offAt[i] = 0;
    setRelay(i, false);   // start OFF
    Serial.printf("[INIT] %-6s (%s) on GPIO %2d ready, starts OFF\n", names[i], types[i], pins[i]);
  }

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[WIFI] Access point '%s' %s\n", AP_SSID, ok ? "STARTED" : "FAILED");
  Serial.print("[WIFI] Connect phone to it, then open http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() {
    Serial.println("[HTTP] Page requested");
    server.send_P(200, "text/html", PAGE);
  });
  server.on("/api/status", sendStatus);   // not logged: the page polls it every 1.5s
  server.on("/api/set", handleSet);
  server.on("/api/all", handleAll);
  server.begin();
  Serial.println("[HTTP] Web server started");

  if (SELF_TEST) selfTest();
  Serial.println("Serial commands: 1/2/3 = toggle, a = all ON, o = all OFF, s = status");
}

void loop() {
  server.handleClient();
  handleSerial();

  // report phones connecting / leaving the ESP32 Wi-Fi
  int st = WiFi.softAPgetStationNum();
  if (st != lastStations) {
    Serial.printf("[WIFI] Devices connected: %d\n", st);
    lastStations = st;
  }

  // auto-off timers
  for (int i = 0; i < N; i++) {
    if (state[i] && offAt[i] > 0 && (long)(millis() - offAt[i]) >= 0) {
      Serial.printf("[TIMER] %s auto-off reached\n", names[i]);
      setRelay(i, false);
      offAt[i] = 0;
    }
  }
}
