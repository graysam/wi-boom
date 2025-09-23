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
#include <FS.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static Preferences prefs;

struct FireConfig {
  uint8_t  count;           // pulses per burst (1..10)
  uint16_t rateHz;          // frequency (2..150 Hz)
  uint8_t  repeats;         // number of bursts (1..6)
  uint32_t repeatSpacing;   // ms between bursts
  bool     continuous;      // continuous stream override
  uint32_t width;           // pulse width (ms) — system setting
  bool     timerEnabled;    // timer mode enabled
  uint32_t timerMs;         // timer delay ms
};

static FireConfig g_cfg;       // current editable config
static FireConfig g_fire;      // locked in when armed
static volatile bool g_armed = false;
static volatile bool g_pulseActive = false;
static volatile bool g_timerActive = false;
static uint32_t      g_timerDeadline = 0; // millis when timer ends
static uint32_t      g_pageLoadCount = 0;
static bool          g_allowUnsupervisedTimer = false; // default safety

// ---------------------------------------------------------------------------
// Configuration persistence
static void loadPrefs() {
  // New keys (with sensible defaults)
  g_cfg.count          = prefs.getUChar ("count",   1);
  g_cfg.rateHz         = prefs.getUShort("rateHz",  10);
  g_cfg.repeats        = prefs.getUChar ("repeats", 1);
  g_cfg.repeatSpacing  = prefs.getUInt  ("repSpa",  500);
  g_cfg.continuous     = prefs.getBool  ("cont",    false);
  g_cfg.width          = prefs.getUInt  ("width",   DEFAULT_PULSE_WIDTH_MS);
  g_cfg.timerEnabled   = prefs.getBool  ("tmrE",    false);
  g_cfg.timerMs        = prefs.getUInt  ("tmrMs",   10000);
  g_allowUnsupervisedTimer = prefs.getBool("tmrUnsv", false);

  // Legacy migration (buzz/spacing/repeat)
  if (prefs.isKey("repeat") || prefs.isKey("spacing") || prefs.isKey("buzz")) {
    bool legacyBuzz     = prefs.getBool ("buzz", false);
    uint32_t legacySp   = prefs.getUInt ("spacing", DEFAULT_BUZZ_SPACING_MS);
    uint8_t legacyRep   = prefs.getUChar("repeat",  DEFAULT_BUZZ_REPEAT);
    if (legacyBuzz) {
      g_cfg.count  = 10; // old buzz loop had 10 pulses
      g_cfg.rateHz = legacySp ? (uint16_t)max<uint32_t>(2, min<uint32_t>(150, 1000 / legacySp)) : 10;
    }
    g_cfg.repeats = legacyRep;
  }
  Serial.printf("Prefs loaded: cnt=%u rate=%uHz reps=%u repSp=%lu cont=%d width=%lu tmrE=%d tmrMs=%lu tmrUnsv=%d\n",
                g_cfg.count, g_cfg.rateHz, g_cfg.repeats, (unsigned long)g_cfg.repeatSpacing,
                (int)g_cfg.continuous, (unsigned long)g_cfg.width,
                (int)g_cfg.timerEnabled, (unsigned long)g_cfg.timerMs, (int)g_allowUnsupervisedTimer);
}

static void savePrefs() {
  prefs.putUChar ("count",   g_cfg.count);
  prefs.putUShort("rateHz",  g_cfg.rateHz);
  prefs.putUChar ("repeats", g_cfg.repeats);
  prefs.putUInt  ("repSpa",  g_cfg.repeatSpacing);
  prefs.putBool  ("cont",    g_cfg.continuous);
  prefs.putUInt  ("width",   g_cfg.width);
  prefs.putBool  ("tmrE",    g_cfg.timerEnabled);
  prefs.putUInt  ("tmrMs",   g_cfg.timerMs);
  prefs.putBool  ("tmrUnsv", g_allowUnsupervisedTimer);
  Serial.printf("Prefs saved: cnt=%u rate=%uHz reps=%u repSp=%lu cont=%d width=%lu tmrE=%d tmrMs=%lu tmrUnsv=%d\n",
                g_cfg.count, g_cfg.rateHz, g_cfg.repeats, (unsigned long)g_cfg.repeatSpacing,
                (int)g_cfg.continuous, (unsigned long)g_cfg.width,
                (int)g_cfg.timerEnabled, (unsigned long)g_cfg.timerMs, (int)g_allowUnsupervisedTimer);
}

// ---------------------------------------------------------------------------
// Inline admin page (immutable). UI is served from SPIFFS.
static const char ADMIN_HTML[] PROGMEM = R"HTML(<!doctype html><html lang=\"en\"><head>
<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\">
<meta name=\"apple-mobile-web-app-capable\" content=\"yes\"><meta name=\"apple-mobile-web-app-status-bar-style\" content=\"black\">
<title>PeRci Admin</title>
<style>body{font:16px/1.4 system-ui,sans-serif;margin:0;padding:24px;background:#0b1021;color:#e8f0ff}h1{margin:0 0 12px}section{background:#11162d;border-radius:12px;padding:16px;margin:16px 0}label{display:block;margin:8px 0}input[type=number]{width:120px}button{padding:10px 16px;border:0;border-radius:8px;background:#19c37d;color:#03152b;font-weight:800;cursor:pointer}button.secondary{background:#334}.row{display:flex;align-items:center;gap:12px;flex-wrap:wrap}.ok{color:#19c37d}.warn{color:#ffb000}.err{color:#ff4d4d}</style>
</head><body>
<h1>PeRci • Admin</h1>
<section><h3>System Settings</h3>
  <label>Pulse Width (ms) <input id=\"width\" type=\"number\" min=\"1\" max=\"1000\"></label>
  <div class=\"row\"><label><input id=\"unsv\" type=\"checkbox\"> Allow unsupervised timer (continues if UI disconnects)</label></div>
  <div class=\"row\"><button id=\"save\">Save</button> <span id=\"saveMsg\"></span></div>
</section>
<section><h3>UI Assets</h3>
  <div>Web UI is served from on-device storage (SPIFFS). Use “Check for updates” to fetch the latest UI bundle from GitHub.</div>
  <div class=\"row\"><button id=\"check\">Check for updates</button><button id=\"apply\" class=\"secondary\">Apply update</button> <span id=\"upMsg\"></span></div>
</section>
<section><h3>Open UI</h3>
  <div class=\"row\"><a href=\"/\" style=\"color:#9cf\">Open main UI</a></div>
</section>
<script>(function(){async function g(p){const r=await fetch(p);if(!r.ok)throw new Error('HTTP '+r.status);return r.json()}async function j(p,b){const r=await fetch(p,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify(b)});if(!r.ok)throw new Error('HTTP '+r.status);return r.json()}async function load(){const s=await g('/api/settings');width.value=s.width;unsv.checked=!!s.allowUnsupervisedTimer}save.onclick=async()=>{saveMsg.textContent='';try{await j('/api/settings',{width:+width.value,allowUnsupervisedTimer:unsv.checked});saveMsg.textContent='Saved';saveMsg.className='ok'}catch(e){saveMsg.textContent='Error';saveMsg.className='err'}};check.onclick=async()=>{upMsg.textContent='';try{const r=await g('/api/ui-update/check');upMsg.textContent=r.ok?`Available: files=${r.files} size=${r.size}B free=${r.free}B`:'No update info';upMsg.className=r.ok?'ok':'warn'}catch(e){upMsg.textContent='Error';upMsg.className='err'}};apply.onclick=async()=>{upMsg.textContent='';try{const r=await j('/api/ui-update/apply',{});upMsg.textContent=r.ok?'Updated UI':(r.err||'Failed');upMsg.className=r.ok?'ok':'err'}catch(e){upMsg.textContent='Error';upMsg.className='err'}};load()})();</script>
</body></html>)HTML";

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
    Serial.printf("Action: ARM on=true (cnt=%u rate=%uHz reps=%u repSp=%lums cont=%d width=%lums)\n",
                  g_fire.count, g_fire.rateHz, g_fire.repeats, (unsigned long)g_fire.repeatSpacing,
                  (int)g_fire.continuous, (unsigned long)g_fire.width);
  } else if (!enabled) {
    g_armed = false;
    g_timerActive = false;
    Serial.println("Action: ARM on=false");
  }
  return true;
}

static bool actionConfig(const JsonVariantConst &doc) {
  if (g_armed) return false; // no changes while armed
  g_cfg.count         = doc["count"]    | g_cfg.count;
  g_cfg.rateHz        = doc["rateHz"]   | g_cfg.rateHz;
  g_cfg.repeats       = doc["repeats"]  | g_cfg.repeats;
  g_cfg.repeatSpacing = doc["repSpa"]   | g_cfg.repeatSpacing;
  g_cfg.continuous    = doc["continuous"] | g_cfg.continuous;
  g_cfg.timerEnabled  = doc["timer"]    | g_cfg.timerEnabled;
  g_cfg.timerMs       = doc["timerMs"]  | g_cfg.timerMs;
  g_cfg.width         = doc["width"]    | g_cfg.width; // allow updates from admin
  savePrefs();
  Serial.printf("Action: CFG cnt=%u rate=%uHz reps=%u repSp=%lums cont=%d tmrE=%d tmrMs=%lu width=%lu\n",
                g_cfg.count, g_cfg.rateHz, g_cfg.repeats, (unsigned long)g_cfg.repeatSpacing,
                (int)g_cfg.continuous, (int)g_cfg.timerEnabled, (unsigned long)g_cfg.timerMs, (unsigned long)g_cfg.width);
  return true;
}

static void pulsesOnce(uint32_t widthMs) {
  digitalWrite(PIN_PULSE_OUT, HIGH);
  digitalWrite(PIN_LED_PULSE, HIGH);
  vTaskDelay(pdMS_TO_TICKS(widthMs));
  digitalWrite(PIN_PULSE_OUT, LOW);
  digitalWrite(PIN_LED_PULSE, LOW);
}

static void fireTask(void *) {
  const uint16_t rate = max<uint16_t>(2, min<uint16_t>(150, g_fire.rateHz));
  const uint32_t periodMs = 1000UL / rate;
  const uint32_t widthMs = min(g_fire.width, periodMs > 1 ? periodMs - 1 : g_fire.width);

  if (g_fire.continuous) {
    Serial.println("Fire: continuous start");
    while (g_pulseActive) {
      pulsesOnce(widthMs);
      const uint32_t offMs = periodMs > widthMs ? (periodMs - widthMs) : 1;
      vTaskDelay(pdMS_TO_TICKS(offMs));
    }
    Serial.println("Fire: continuous stop");
  } else {
    for (uint8_t r = 0; r < g_fire.repeats && g_pulseActive; ++r) {
      for (uint8_t i = 0; i < g_fire.count && g_pulseActive; ++i) {
        pulsesOnce(widthMs);
        const uint32_t offMs = periodMs > widthMs ? (periodMs - widthMs) : 1;
        if (i < g_fire.count - 1) vTaskDelay(pdMS_TO_TICKS(offMs));
      }
      if (r < g_fire.repeats - 1 && g_pulseActive) vTaskDelay(pdMS_TO_TICKS(g_fire.repeatSpacing));
    }
  }
  g_pulseActive = false;
  g_armed = false;
  Serial.println("Action: FIRE completed; auto-disarm");
  vTaskDelete(nullptr);
}

bool actionFire() {
  if (!g_armed) return false;
  // Timer mode
  if (g_cfg.timerEnabled && g_cfg.timerMs > 0 && !g_timerActive && !g_pulseActive) {
    g_timerActive = true;
    g_timerDeadline = millis() + g_cfg.timerMs;
    Serial.printf("Action: TIMER start %lums\n", (unsigned long)g_cfg.timerMs);
    xTaskCreate([](void*){
      while (g_timerActive) {
        // Safety: cancel if UI disconnects and unsupervised not allowed
        if (!g_allowUnsupervisedTimer && ws.count() == 0) {
          Serial.println("TIMER: cancelled due to lost supervision");
          g_timerActive = false; g_armed = false; broadcastState();
          vTaskDelete(nullptr); return; }
        const uint32_t now = millis();
        if ((int32_t)(g_timerDeadline - now) <= 0) break;
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      if (g_timerActive && g_armed) {
        g_timerActive = false;
        g_pulseActive = true;
        Serial.println("Action: FIRE start (after timer)");
        xTaskCreate(fireTask, "fire", 4096, nullptr, 1, nullptr);
      }
      vTaskDelete(nullptr);
    }, "timerWait", 3072, nullptr, 1, nullptr);
    return true;
  }

  // Continuous toggle
  if (g_cfg.continuous) {
    if (g_pulseActive) {
      g_pulseActive = false; // stop loop
      Serial.println("Action: FIRE stop (continuous toggle)");
      return true;
    }
  }
  if (g_pulseActive) return false;
  g_pulseActive = true;
  Serial.println("Action: FIRE start");
  xTaskCreate(fireTask, "fire", 4096, nullptr, 1, nullptr);
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
  } else if (!strcmp(cmd, "timeSync")) {
    StaticJsonDocument<128> td; td["type"]="time"; td["nowMs"]=millis(); char out[128]; size_t n=serializeJson(td,out,sizeof(out)); ws.textAll(out,n);
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
      if (!g_allowUnsupervisedTimer && g_timerActive && ws.count() <= 1) {
        g_timerActive = false;
        g_armed = false;
        Serial.println("TIMER: cancelled on disconnect");
      }
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
static void onAdmin(AsyncWebServerRequest *req) {
  AsyncWebServerResponse *res = req->beginResponse_P(200, "text/html; charset=utf-8", ADMIN_HTML);
  res->addHeader(
    "Content-Security-Policy",
    "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'unsafe-inline' 'self'; connect-src 'self' ws: wss:;"
  );
  req->send(res);
}

static void apiSettingsGet(AsyncWebServerRequest *req) {
  StaticJsonDocument<192> d; d["width"] = g_cfg.width; d["allowUnsupervisedTimer"] = g_allowUnsupervisedTimer; char out[192]; size_t n = serializeJson(d,out,sizeof(out)); req->send(200, "application/json", String(out,n));
}
static void apiSettingsPost(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
  StaticJsonDocument<256> d; DeserializationError e = deserializeJson(d, data, len); if (e) { req->send(400, "application/json", "{\"ok\":false}"); return; }
  if (!g_armed) { g_cfg.width = d["width"] | g_cfg.width; }
  g_allowUnsupervisedTimer = d["allowUnsupervisedTimer"] | g_allowUnsupervisedTimer; savePrefs();
  req->send(200, "application/json", "{\"ok\":true}");
}

static const char *UPD_BASE = "https://raw.githubusercontent.com/graysam/wi-boom/2.0.1-development/webroot";
static bool httpGetToFile(const String &url, const String &path) {
  WiFiClientSecure cli; cli.setInsecure(); HTTPClient http; if (!http.begin(cli, url)) return false; int code = http.GET(); if (code != HTTP_CODE_OK) { http.end(); return false; }
  File f = SPIFFS.open(path, FILE_WRITE); if (!f) { http.end(); return false; }
  WiFiClient *s = http.getStreamPtr(); uint8_t buf[1024]; int r;
  while ((r = s->readBytes((char*)buf, sizeof(buf))) > 0) { if (f.write(buf, r) != (size_t)r) { f.close(); http.end(); return false; } }
  f.close(); http.end(); return true;
}
static void apiUiCheck(AsyncWebServerRequest *req) {
  WiFiClientSecure cli; cli.setInsecure(); HTTPClient http; String mu = String(UPD_BASE) + "/manifest.json"; bool ok=false; size_t files=0, size=0, freeB=SPIFFS.totalBytes()? (SPIFFS.totalBytes()-SPIFFS.usedBytes()) : 0;
  if (http.begin(cli, mu)) { int code = http.GET(); if (code == HTTP_CODE_OK) { StaticJsonDocument<1024> d; DeserializationError e = deserializeJson(d, http.getString()); if (!e) { ok=true; files = d["files"].size(); for (JsonObject it : d["files"].as<JsonArray>()) size += (size_t)(it["size"].as<unsigned long>()); } } http.end(); }
  StaticJsonDocument<192> out; out["ok"]=ok; out["files"]=files; out["size"]=size; out["free"]=freeB; String s; serializeJson(out, s); req->send(200, "application/json", s);
}
static void apiUiApply(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
  (void)data; (void)len; bool ok=false; String err=""; do {
    WiFiClientSecure cli; cli.setInsecure(); HTTPClient http; String mu = String(UPD_BASE) + "/manifest.json";
    if (!http.begin(cli, mu)) { err="manifest begin"; break; }
    int code = http.GET(); if (code != HTTP_CODE_OK) { http.end(); err="manifest http"; break; }
    StaticJsonDocument<4096> d; DeserializationError e = deserializeJson(d, http.getString()); http.end(); if (e) { err = "manifest parse"; break; }
    size_t need=0; for (JsonObject it : d["files"].as<JsonArray>()) need += (size_t)(it["size"].as<unsigned long>());
    size_t freeB = SPIFFS.totalBytes()? (SPIFFS.totalBytes()-SPIFFS.usedBytes()) : 0; if (freeB + 10240 < need) { err = "insufficient space"; break; }
    SPIFFS.mkdir("/__new");
    for (JsonObject it : d["files"].as<JsonArray>()) {
      String p = String(it["path"].as<const char*>());
      String url = String(UPD_BASE) + "/" + p;
      String dst = String("/__new/") + p;
      for (int i=1;i<dst.length();++i){ if (dst[i]=='/') { SPIFFS.mkdir(dst.substring(0,i)); } }
      if (!httpGetToFile(url, dst)) { err = String("download ")+p; break; }
    }
    if (err.length()) break;
    for (JsonObject it : d["files"].as<JsonArray>()) {
      String rel = String(it["path"].as<const char*>());
      String dst = String("/webroot/") + rel;
      if (SPIFFS.exists(dst)) SPIFFS.remove(dst);
      for (int i=1;i<dst.length();++i){ if (dst[i]=='/') { SPIFFS.mkdir(dst.substring(0,i)); } }
      String src = String("/__new/") + rel;
      SPIFFS.rename(src, dst);
    }
    ok=true;
  } while(false);
  StaticJsonDocument<192> out; out["ok"]=ok; if(!ok) out["err"]=err; String s; serializeJson(out, s); req->send(200, "application/json", s);
}

void initWeb() {
  prefs.begin("hv", false);
  loadPrefs();
  Serial.println(F("initWeb(): prefs ready, mounting routes"));

  if (!SPIFFS.begin(true)) {
    Serial.println(F("SPIFFS mount failed"));
  } else {
    Serial.printf("SPIFFS: total=%lu used=%lu\n", (unsigned long)SPIFFS.totalBytes(), (unsigned long)SPIFFS.usedBytes());
  }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/admin", HTTP_GET, onAdmin);
  server.on("/api/settings", HTTP_GET, apiSettingsGet);
  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *req){}, NULL, apiSettingsPost);
  server.on("/api/ui-update/check", HTTP_GET, apiUiCheck);
  server.on("/api/ui-update/apply", HTTP_POST, [](AsyncWebServerRequest *req){}, NULL, apiUiApply);

  if (SPIFFS.exists("/webroot/index.html")) {
    server.serveStatic("/", SPIFFS, "/webroot/").setDefaultFile("index.html");
  } else {
    server.on("/", HTTP_GET, onAdmin);
  }

  server.onNotFound([](AsyncWebServerRequest *req) {
    if (SPIFFS.exists("/webroot/index.html")) req->redirect("/"); else req->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println(F("HTTP server (async) started"));
}

// ---------------------------------------------------------------------------
// Telemetry
void broadcastState() {
  StaticJsonDocument<384> doc;
  doc["type"]        = "state";
  doc["pageCount"]   = g_pageLoadCount;
  doc["armed"]       = g_armed;
  doc["pulseActive"] = g_pulseActive;
  JsonObject cfg = doc.createNestedObject("cfg");
  cfg["count"]      = g_cfg.count;
  cfg["rateHz"]     = g_cfg.rateHz;
  cfg["repeats"]    = g_cfg.repeats;
  cfg["repSpa"]     = g_cfg.repeatSpacing;
  cfg["continuous"] = g_cfg.continuous;
  cfg["width"]      = g_cfg.width;
  cfg["timer"]      = g_cfg.timerEnabled;
  cfg["timerMs"]    = g_cfg.timerMs;
  doc["wifiClients"] = WiFi.softAPgetStationNum();
  doc["wifiConnected"] = (ws.count() > 0);
  doc["wsCount"] = ws.count();
  doc["apSSID"] = WIFI_AP_SSID;
  doc["staConnected"] = (WiFi.getMode() == WIFI_AP_STA && WiFi.status() == WL_CONNECTED);
  doc["staIP"] = (WiFi.getMode() == WIFI_AP_STA && WiFi.status() == WL_CONNECTED)
                   ? WiFi.localIP().toString() : String("");
  doc["adc"] = 0;
  doc["nowMs"] = millis();
  if (g_timerActive) {
    JsonObject t = doc.createNestedObject("timer");
    t["active"] = true;
    t["deadlineMs"] = g_timerDeadline;
  }

  char out[384];
  const size_t n = serializeJson(doc, out, sizeof(out));
  ws.textAll(out, n);
}

// Settings helpers
void setAllowUnsupervisedTimer(bool allow) { g_allowUnsupervisedTimer = allow; savePrefs(); }
bool getAllowUnsupervisedTimer() { return g_allowUnsupervisedTimer; }

