// ============================================================================
// file: web_server.cpp
// Async web server, websocket, routes, actions, and indicator management.
// ============================================================================

#include "web_server.h"
#include "config.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static Preferences prefs;

struct FireConfig {
  bool     buzz;
  uint32_t width;
  uint32_t spacing;
  uint8_t  repeat;
};

static FireConfig g_cfg;       // current editable config
static FireConfig g_fire;      // locked in when armed
static volatile bool g_armed = false;
static volatile bool g_pulseActive = false;
static uint32_t      g_pageLoadCount = 0;

// ---------------------------------------------------------------------------
// Configuration persistence
static void loadPrefs() {
  g_cfg.buzz    = prefs.getBool("buzz", false);
  g_cfg.width   = prefs.getUInt("width",   DEFAULT_PULSE_WIDTH_MS);
  g_cfg.spacing = prefs.getUInt("spacing", DEFAULT_BUZZ_SPACING_MS);
  g_cfg.repeat  = prefs.getUChar("repeat", DEFAULT_BUZZ_REPEAT);
  Serial.printf("Prefs loaded: mode=%s width=%lu spacing=%lu repeat=%u\n",
                g_cfg.buzz ? "buzz" : "single",
                (unsigned long)g_cfg.width,
                (unsigned long)g_cfg.spacing,
                (unsigned)g_cfg.repeat);
}

static void savePrefs() {
  prefs.putBool("buzz", g_cfg.buzz);
  prefs.putUInt("width", g_cfg.width);
  prefs.putUInt("spacing", g_cfg.spacing);
  prefs.putUChar("repeat", g_cfg.repeat);
  Serial.printf("Prefs saved: mode=%s width=%lu spacing=%lu repeat=%u\n",
                g_cfg.buzz ? "buzz" : "single",
                (unsigned long)g_cfg.width,
                (unsigned long)g_cfg.spacing,
                (unsigned)g_cfg.repeat);
}

// ---------------------------------------------------------------------------
// Minimal inline UI (served from flash)
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HV Trigger</title>
<style>
 html,body{margin:0;height:100%;overflow:hidden;font:16px/1.4 system-ui,sans-serif;background:#f6f7f9;}
 .wrap{display:flex;flex-direction:column;height:100%;align-items:center;justify-content:center;padding:1rem;box-sizing:border-box;}
 .controls{width:100%;max-width:480px;}
 label{display:block;margin-top:12px;}
 input[type=range],select{width:100%;}
 #fire{margin:2rem auto;border-radius:50%;width:200px;height:200px;background:#ff4d4d;color:#fff;font-size:2rem;border:none;opacity:.5;}
 #fire.enabled{opacity:1;}
 .row{display:flex;gap:12px;justify-content:center;}
 button.small{padding:12px 16px;border:0;border-radius:8px;background:#fff;box-shadow:0 2px 8px rgba(0,0,0,.08);font-weight:600;}
 .statusbar{position:fixed;left:0;right:0;bottom:28px;height:34px;background:#0b1021;color:#e8f0ff;display:flex;align-items:center;justify-content:center;font:14px/1.2 ui-monospace,Consolas,monospace;gap:16px}
 .statusbar span{margin:0 8px;}
 .infobar{position:fixed;left:0;right:0;bottom:0;height:28px;background:rgba(11,16,33,.9);color:#c8d6ff;display:flex;align-items:center;justify-content:center;font:12px/1.2 ui-monospace,Consolas,monospace}
 .ctrl{display:flex;align-items:center;gap:12px;margin-top:12px}
 .val{min-width:64px;text-align:center;padding:6px 8px;border-radius:8px;background:#0b1021;color:#e8f0ff;font-weight:700}
 button.small.active{background:#0b1021;color:#e8f0ff}
 .led{width:14px;height:14px;border-radius:50%;background:#711;box-shadow:0 0 0 2px rgba(255,255,255,.1) inset,0 0 8px rgba(0,0,0,.4)}
 .green{background:#19c37d}
 .amber{background:#ffb000}
 .red{background:#ff4d4d}
 .blink{animation:blink 1s infinite ease-in-out}
 @keyframes blink{0%{opacity:.4}50%{opacity:1}100%{opacity:.4}}
 .mode{font-weight:800}
 .triplet{font-weight:700}
 .veil{position:fixed;inset:0;background:rgba(0,0,0,.45);display:flex;align-items:center;justify-content:center;color:#fff;font-weight:800;letter-spacing:.1em}
 .hidden{display:none}
 .sp{opacity:.7}
</style>
</head>
<body>
<div id="veil" class="veil hidden">RECONNECTING...</div>
<div class="wrap">
  <div class="controls">
    <label>Mode
      <select id="mode">
        <option value="single">Single</option>
        <option value="buzz">Buzz</option>
      </select>
    </label>
    <div class="ctrl"><div class="val" id="widthVal">10ms</div><label style="flex:1">PULSE WIDTH (ms) <input id="width" type="range" min="5" max="100" value="10"></label></div>
    <div class="ctrl"><div class="val" id="spacingVal">20ms</div><label style="flex:1">BUZZ SPACING (ms) <input id="spacing" type="range" min="10" max="100" value="20"></label></div>
    <div class="ctrl"><div class="val" id="repeatVal">1x</div><label style="flex:1">REPETITIONS <input id="repeat" type="range" min="1" max="4" value="1"></label></div>
  </div>

  <button id="fire" disabled>FIRE</button>
  <div class="row">
    <button id="arm" class="small">Arm</button>
    <button id="disarm" class="small">Disarm</button>
  </div>
</div>

<div class="statusbar">
  <span class="led red" id="led-ws" title="WebSocket"></span>
  <span class="led red" id="led-armed" title="Armed"></span>
  <span class="mode" id="modeLabel">SINGLE-SHOT</span>
  <span class="triplet" id="triplet">10/20/1</span>
  <span class="sp" id="apName">-</span>
</div>
<div class="infobar" id="infobar">Idle.</div>

<script>
(()=>{
  const $=id=>document.getElementById(id);
  const mode=$("mode"), width=$("width"), spacing=$("spacing"), repeat=$("repeat");
  const widthVal=$("widthVal"), spacingVal=$("spacingVal"), repeatVal=$("repeatVal");
  const fire=$("fire"), arm=$("arm"), disarm=$("disarm");
  const ledWs=$("led-ws"), ledArmed=$("led-armed"), modeLabel=$("modeLabel"), triplet=$("triplet"), apName=$("apName"), infobar=$("infobar"), veil=$("veil");
  const state={armed:false, connected:false};
  const proto=location.protocol==="https:"?"wss":"ws";
  let ws=null; let reconnectTimer=null;

  function cls(el, on, name){ el.classList[on?"add":"remove"](name); }
  function setLed(el, color, blink){ el.className = `led ${color}` + (blink?" blink":""); }
  function setArmedUI(on){
    state.armed=!!on;
    if(!state.armed){
      fire.disabled=true;fire.classList.remove("enabled");
      [mode,width,spacing,repeat].forEach(el=>el.disabled=false);
      cls(arm,true,"active"); arm.disabled=false; cls(disarm,false,"active"); disarm.disabled=true;
      setLed(ledArmed, "red", false);
    }else{
      fire.disabled=false;fire.classList.add("enabled");
      [mode,width,spacing,repeat].forEach(el=>el.disabled=true);
      cls(arm,false,"active"); arm.disabled=true; cls(disarm,true,"active"); disarm.disabled=false;
      setLed(ledArmed, "amber", true);
    }
  }

  function updateValueDisplays(){
    widthVal.textContent = `${width.value}ms`;
    spacingVal.textContent = `${spacing.value}ms`;
    repeatVal.textContent = `${repeat.value}x`;
    triplet.textContent = `${width.value}/${spacing.value}/${repeat.value}`;
  }

  function applyState(m){
    setArmedUI(m.armed);
    mode.value=m.cfg.mode;
    width.value=m.cfg.width;
    spacing.value=m.cfg.spacing;
    repeat.value=m.cfg.repeat;
    modeLabel.textContent = m.cfg.mode==="buzz"?"BUZZ":"SINGLE-SHOT";
    apName.textContent = m.apSSID || "-";
    updateValueDisplays();
  }

  function sendCfg(){
    if(!ws || ws.readyState!==1 || state.armed) return;
    ws.send(JSON.stringify({cmd:"cfg",mode:mode.value,width:+width.value,spacing:+spacing.value,repeat:+repeat.value}));
  }

  [mode,width,spacing,repeat].forEach(el=>{
    el.addEventListener("input",()=>{ updateValueDisplays(); sendCfg(); });
  });
  arm.onclick=()=>{ if(ws && ws.readyState===1) ws.send(JSON.stringify({cmd:"arm",on:true})); };
  disarm.onclick=()=>{ if(ws && ws.readyState===1) ws.send(JSON.stringify({cmd:"arm",on:false})); };
  fire.onclick=()=>{ if(ws && ws.readyState===1) ws.send(JSON.stringify({cmd:"fire"})); };

  function connectWs(){
    clearTimeout(reconnectTimer);
    setLed(ledWs, "amber", true); veil.classList.remove("hidden"); infobar.textContent = "Connecting...";
    try { ws && ws.close && ws.close(); } catch(e){}
    ws = new WebSocket(`${proto}://${location.host}/ws`);
    ws.onopen = ()=>{ state.connected=true; setLed(ledWs, "green", false); veil.classList.add("hidden"); infobar.textContent = "Connected."; };
    ws.onmessage = ev=>{
      try{
        const m=JSON.parse(ev.data);
        if(m.type==="state"){ applyState(m); }
      }catch(e){ /* ignore */ }
    };
    function schedule(){
      if(reconnectTimer) return;
      reconnectTimer = setTimeout(()=>{ reconnectTimer=null; connectWs(); }, 1000);
    }
    ws.onerror = ()=>{ state.connected=false; setLed(ledWs, "red", false); veil.classList.remove("hidden"); infobar.textContent = "Connection error. Retrying..."; schedule(); };
    ws.onclose = ()=>{ state.connected=false; setLed(ledWs, "red", false); veil.classList.remove("hidden"); infobar.textContent = "Disconnected. Retrying..."; schedule(); };
  }

  updateValueDisplays();
  connectWs();
})();
</script>
</body>
</html>)HTML";

// ---------------------------------------------------------------------------
// Wi-Fi AP / AP+STA setup
void setupWiFiAP() {
  IPAddress ip(AP_IP[0], AP_IP[1], AP_IP[2], AP_IP[3]);
  IPAddress gw(AP_GW[0], AP_GW[1], AP_GW[2], AP_GW[3]);
  IPAddress mask(AP_MASK[0], AP_MASK[1], AP_MASK[2], AP_MASK[3]);

  if (WIFI_APSTA && strlen(STA_SSID) > 0) {
    WiFi.mode(WIFI_AP_STA);
    Serial.printf("WiFi: starting AP+STA (ssid=%s)\n", STA_SSID);
  } else {
    WiFi.mode(WIFI_AP);
    Serial.println(F("WiFi: starting AP-only"));
  }

  if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS)) {
    Serial.println(F("SoftAP start failed"));
  }
  if (!WiFi.softAPConfig(ip, gw, mask)) {
    Serial.println(F("SoftAP config failed"));
  }
  Serial.printf("AP up: %s  IP: %s\n", WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());

  if (WiFi.getMode() == WIFI_AP_STA && strlen(STA_SSID) > 0) {
    WiFi.begin(STA_SSID, STA_PASS);
    const uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 10000) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("STA joined: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println(F("STA join timeout"));
    }
  }
}

// ---------------------------------------------------------------------------
// Indicator management
void updateIndicators() {
  const uint32_t now = millis();

  // Network LED: steady green if WS client(s), else amber flash if no AP clients, else alt green/amber
  static uint32_t lastNet = 0; static bool netToggle = false;
  if (now - lastNet >= 500) {
    lastNet = now;
    ws.cleanupClients();
    uint8_t sta = WiFi.softAPgetStationNum();
    if (ws.count() > 0) {
      digitalWrite(PIN_LED_AMBER, LOW);
      digitalWrite(PIN_LED_GREEN, HIGH);
    } else if (sta == 0) {
      netToggle = !netToggle;
      digitalWrite(PIN_LED_AMBER, netToggle);
      digitalWrite(PIN_LED_GREEN, LOW);
    } else {
      netToggle = !netToggle;
      digitalWrite(PIN_LED_AMBER, netToggle);
      digitalWrite(PIN_LED_GREEN, !netToggle);
    }
  }

  // Armed LED: slow flash when armed
  static uint32_t lastArm = 0; static bool armToggle = false;
  if (!g_armed) {
    digitalWrite(PIN_LED_ARMED, LOW);
  } else if (now - lastArm >= 1000) {
    lastArm = now;
    armToggle = !armToggle;
    digitalWrite(PIN_LED_ARMED, armToggle);
  }
}

// ---------------------------------------------------------------------------
// Actions
bool actionArm(bool enabled) {
  if (enabled && !g_armed) {
    g_fire = g_cfg; // lock in current config
    g_armed = true;
    Serial.printf("Action: ARM on=true (mode=%s w=%lu s=%lu r=%u)\n",
                  g_fire.buzz?"buzz":"single",
                  (unsigned long)g_fire.width,(unsigned long)g_fire.spacing,(unsigned)g_fire.repeat);
  } else if (!enabled) {
    g_armed = false;
    Serial.println("Action: ARM on=false");
  }
  return true;
}

static bool actionConfig(const JsonVariantConst &doc) {
  if (g_armed) return false; // no changes while armed
  const char *mode = doc["mode"] | (g_cfg.buzz ? "buzz" : "single");
  g_cfg.buzz    = !strcmp(mode, "buzz");
  g_cfg.width   = doc["width"]   | g_cfg.width;
  g_cfg.spacing = doc["spacing"] | g_cfg.spacing;
  g_cfg.repeat  = doc["repeat"]  | g_cfg.repeat;
  savePrefs();
  Serial.printf("Action: CFG mode=%s w=%lu s=%lu r=%u\n",
                g_cfg.buzz?"buzz":"single",
                (unsigned long)g_cfg.width,(unsigned long)g_cfg.spacing,(unsigned)g_cfg.repeat);
  return true;
}

static void fireTask(void *) {
  for (uint8_t r = 0; r < g_fire.repeat; ++r) {
    if (g_fire.buzz) {
      for (uint8_t i = 0; i < 10; ++i) {
        digitalWrite(PIN_PULSE_OUT, HIGH);
        digitalWrite(PIN_LED_PULSE, HIGH);
        vTaskDelay(pdMS_TO_TICKS(g_fire.width));
        digitalWrite(PIN_PULSE_OUT, LOW);
        if (g_fire.width < 50) vTaskDelay(pdMS_TO_TICKS(50 - g_fire.width));
        digitalWrite(PIN_LED_PULSE, LOW);
        if (i < 9) vTaskDelay(pdMS_TO_TICKS(g_fire.spacing));
      }
    } else {
      digitalWrite(PIN_PULSE_OUT, HIGH);
      digitalWrite(PIN_LED_PULSE, HIGH);
      vTaskDelay(pdMS_TO_TICKS(g_fire.width));
      digitalWrite(PIN_PULSE_OUT, LOW);
      if (g_fire.width < 50) vTaskDelay(pdMS_TO_TICKS(50 - g_fire.width));
      digitalWrite(PIN_LED_PULSE, LOW);
    }
    if (r < g_fire.repeat - 1) vTaskDelay(pdMS_TO_TICKS(g_fire.spacing));
  }
  g_pulseActive = false;
  g_armed = false;
  Serial.println("Action: FIRE completed; auto-disarm");
  vTaskDelete(nullptr);
}

bool actionFire() {
  if (!g_armed || g_pulseActive) return false;
  g_pulseActive = true;
  Serial.println("Action: FIRE start");
  xTaskCreate(fireTask, "fire", 2048, nullptr, 1, nullptr);
  return true;
}

// ---------------------------------------------------------------------------
// WebSocket
static void handleWsMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (!info || info->opcode != WS_TEXT) return;

  static char buf[256];
  const size_t n = len < sizeof(buf)-1 ? len : sizeof(buf)-1;
  memcpy(buf, data, n); buf[n] = '\0';

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, buf);
  if (err) { Serial.printf("WS: JSON parse error: %s\n", err.c_str()); return; }

  const char *cmd = doc["cmd"] | "";
  Serial.printf("WS: msg %s\n", buf);
  if (!strcmp(cmd, "arm")) {
    bool on = doc["on"] | false;
    actionArm(on);
  } else if (!strcmp(cmd, "cfg")) {
    actionConfig(doc);
  } else if (!strcmp(cmd, "fire")) {
    actionFire();
  }

  broadcastState();
}

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WS: client %u connected\n", client->id());
      g_pageLoadCount++;
      broadcastState();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WS: client %u disconnected\n", client->id());
      broadcastState();
      break;
    case WS_EVT_DATA:
      handleWsMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// ---------------------------------------------------------------------------
// HTTP
static void onIndex(AsyncWebServerRequest *req) {
  g_pageLoadCount++;
  Serial.printf("HTTP: GET / from %s\n", req->client()->remoteIP().toString().c_str());
  AsyncWebServerResponse *res = req->beginResponse_P(200, "text/html; charset=utf-8", INDEX_HTML);
  // Allow inline script (UI is embedded) and WebSocket connections
  res->addHeader(
    "Content-Security-Policy",
    "default-src 'self'; "
    "script-src 'self' 'unsafe-inline'; "
    "style-src 'unsafe-inline' 'self'; "
    "connect-src 'self' ws: wss:;"
  );
  req->send(res);
}

void initWeb() {
  prefs.begin("hv", false);
  loadPrefs();
  Serial.println(F("initWeb(): prefs ready, mounting routes"));

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, onIndex);
  server.onNotFound([](AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println(F("HTTP server (async) started"));
}

// ---------------------------------------------------------------------------
// Telemetry
void broadcastState() {
  StaticJsonDocument<256> doc;
  doc["type"]        = "state";
  doc["pageCount"]   = g_pageLoadCount;
  doc["armed"]       = g_armed;
  doc["pulseActive"] = g_pulseActive;
  JsonObject cfg = doc.createNestedObject("cfg");
  cfg["mode"]    = g_cfg.buzz ? "buzz" : "single";
  cfg["width"]   = g_cfg.width;
  cfg["spacing"] = g_cfg.spacing;
  cfg["repeat"]  = g_cfg.repeat;
  doc["wifiClients"] = WiFi.softAPgetStationNum();
  doc["wifiConnected"] = (ws.count() > 0);
  doc["wsCount"] = ws.count();
  doc["apSSID"] = WIFI_AP_SSID;
  doc["staConnected"] = (WiFi.getMode() == WIFI_AP_STA && WiFi.status() == WL_CONNECTED);
  doc["staIP"] = (WiFi.getMode() == WIFI_AP_STA && WiFi.status() == WL_CONNECTED)
                   ? WiFi.localIP().toString() : String("");
  doc["adc"] = 0;

  char out[256];
  const size_t n = serializeJson(doc, out, sizeof(out));
  ws.textAll(out, n);
}

