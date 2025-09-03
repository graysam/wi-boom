// ============================================================================
// file: web_server.cpp
// ============================================================================

#include "web_server.h"
#include "config.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static volatile bool g_armed = false;
static volatile bool g_pulseActive = false;
static uint32_t      g_pageLoadCount = 0;

// -------------------- Minimal inline UI (served from flash) --------------------
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HV Trigger</title>
<style>
  :root { font: 16px/1.4 system-ui, sans-serif; color: #111; }
  body { margin: 0; padding: 1.25rem; background: #f6f7f9; }
  .wrap { max-width: 720px; margin: 0 auto; }
  h1 { margin: 0 0 0.5rem 0; }
  .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-top: 16px; }
  button, .card {
    padding: 14px 16px; border: 0; border-radius: 14px; box-shadow: 0 2px 8px rgba(0,0,0,.08);
    background: white;
  }
  button { font-weight: 600; cursor: pointer; }
  .fire { font-size: 1.25rem; }
  .fire.armed { background: #fee; }
  .leds > div { display:flex; align-items:center; justify-content:space-between; }
  .pill { width: 16px; height: 16px; border-radius: 999px; background: #ddd; margin-left: 10px; display:inline-block; }
  .on.green { background:#3cb371; }
  .on.amber { background:#ffb000; }
  .on.blue  { background:#5aa3ff; }
  .on.red   { background:#ff4d4d; }
  .row { display:flex; gap:12px; align-items:center; }
  .badge { padding: 4px 8px; background:#eee; border-radius: 999px; font-size:.9rem; }
  .muted { color:#666; font-size:.9rem; }
</style>
</head>
<body>
<div class="wrap">
  <h1>HV Explosive Trigger</h1>
  <div class="row">
    <div class="badge" id="conn">Connecting…</div>
    <div class="badge">Visits: <span id="visits">0</span></div>
    <div class="muted">ADC<sub>res</sub>: <span id="adc">0</span></div>
  </div>

  <div class="grid">
    <button id="arm" class="card">Arm</button>
    <button id="fire" class="fire card">FIRE</button>

    <div class="card leds">
      <div><span>Wi-Fi: <strong id="wifiStatus">—</strong></span><span class="pill" id="wifiLamp"></span></div>
      <div><span>Armed</span><span class="pill" id="armedLamp"></span></div>
      <div><span>Pulse</span><span class="pill" id="pulseLamp"></span></div>
    </div>

    <div class="card">
      <div class="row"><button id="ledGreen">Set GREEN</button><button id="ledAmber">Set AMBER</button><button id="ledOff">LED OFF</button></div>
    </div>
  </div>
</div>

<script>
(() => {
  const $ = (id)=>document.getElementById(id);
  const conn = $("conn"), visits=$("visits"), adc=$("adc");
  const wifiStatus = $("wifiStatus");
  const wifiLamp = $("wifiLamp"), armedLamp=$("armedLamp"), pulseLamp=$("pulseLamp");
  const armBtn=$("arm"), fireBtn=$("fire");
  const greenBtn=$("ledGreen"), amberBtn=$("ledAmber"), offBtn=$("ledOff");

  const proto = location.protocol === "https:" ? "wss" : "ws";
  const ws = new WebSocket(`${proto}://${location.host}/ws`);

  function lamp(el, on, cls) {
    el.className = "pill" + (on ? ` on ${cls}` : "");
  }

  ws.addEventListener("open", () => { conn.textContent = "Connected"; });
  ws.addEventListener("close", () => { conn.textContent = "Disconnected"; });
  ws.addEventListener("message", (ev) => {
    try {
      const m = JSON.parse(ev.data);
      if (m.type === "state") {
        visits.textContent = m.pageCount ?? 0;
        adc.textContent = m.adc ?? 0;
        wifiStatus.textContent = m.wifi ?? "AP";
        lamp(wifiLamp, m.wifiConnected, m.wifiConnected ? "green" : "amber");
        lamp(armedLamp, !!m.armed, "red");
        lamp(pulseLamp, !!m.pulseActive, "blue");
        fireBtn.classList.toggle("armed", !!m.armed);
        armBtn.textContent = m.armed ? "Disarm" : "Arm";
      }
    } catch (e) { /* ignore */ }
  });

  armBtn.onclick  = () => ws.send(JSON.stringify({cmd:"arm", on: armBtn.textContent==="Arm"}));
  fireBtn.onclick = () => ws.send(JSON.stringify({cmd:"fire"}));
  greenBtn.onclick= () => ws.send(JSON.stringify({cmd:"led", which:"green", on:true}));
  amberBtn.onclick= () => ws.send(JSON.stringify({cmd:"led", which:"amber", on:true}));
  offBtn.onclick  = () => ws.send(JSON.stringify({cmd:"led", which:"off", on:false}));
})();
</script>
</body>
</html>)HTML";

// -------------------- Wi-Fi AP --------------------
void setupWiFiAP() {
  WiFi.mode(WIFI_AP);
  IPAddress ip(AP_IP[0], AP_IP[1], AP_IP[2], AP_IP[3]);
  IPAddress gw(AP_GW[0], AP_GW[1], AP_GW[2], AP_GW[3]);
  IPAddress mask(AP_MASK[0], AP_MASK[1], AP_MASK[2], AP_MASK[3]);
  if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS)) {
    Serial.println(F("SoftAP start failed"));
  }
  if (!WiFi.softAPConfig(ip, gw, mask)) {
    Serial.println(F("SoftAP config failed"));
  }
  Serial.printf("AP up: %s  IP: %s\n", WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
  setStatusLED(LED_AMBER); // until a client is connected
}

// -------------------- Actions --------------------
bool actionArm(bool enabled) {
  g_armed = enabled;
  digitalWrite(PIN_LED_ARMED, enabled ? HIGH : LOW);
  return true;
}

bool actionSetIndicator(const String &which, bool on) {
  if (which == "green") {
    setStatusLED(on ? LED_GREEN : LED_OFF);
    return true;
  }
  if (which == "amber") {
    setStatusLED(on ? LED_AMBER : LED_OFF);
    return true;
  }
  if (which == "off") {
    setStatusLED(LED_OFF);
    return true;
  }
  return false;
}

bool actionFire() {
  if (!g_armed) return false; // hard interlock
  if (g_pulseActive) return false;

  g_pulseActive = true;
  digitalWrite(PIN_PULSE_OUT, HIGH);
  digitalWrite(PIN_LED_PULSE, HIGH);

  const uint32_t start = millis();
  // Non-blocking wait to keep WS responsive (short bounded loop)
  while (millis() - start < PULSE_WIDTH_MS) {
    // allow background tasks
    delay(1);
  }

  digitalWrite(PIN_PULSE_OUT, LOW);
  digitalWrite(PIN_LED_PULSE, LOW);
  g_pulseActive = false;
  return true;
}

// -------------------- WebSocket --------------------
static void handleWsMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (!info || info->opcode != WS_TEXT) return;

  // Ensure zero-terminated for simple parse
  static char buf[256];
  const size_t n = len < sizeof(buf)-1 ? len : sizeof(buf)-1;
  memcpy(buf, data, n); buf[n] = '\0';

  // JSON parse
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, buf);
  if (err) return;

  const char *cmd = doc["cmd"] | "";
  if (!strcmp(cmd, "arm")) {
    bool on = doc["on"] | false;
    actionArm(on);
  } else if (!strcmp(cmd, "led")) {
    const char *which = doc["which"] | "off";
    bool on = doc["on"] | false;
    actionSetIndicator(String(which), on);
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
      setStatusLED(LED_GREEN);
      g_pageLoadCount++; // count "active client" as a visit
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WS: client %u disconnected\n", client->id());
      // If no clients remain, show amber
      if (server->count() == 0) setStatusLED(LED_AMBER);
      break;
    case WS_EVT_DATA:
      handleWsMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// -------------------- HTTP --------------------
static void onIndex(AsyncWebServerRequest *req) {
  // Increment load count on initial HTTP hit (separate from WS connect)
  g_pageLoadCount++;
  AsyncWebServerResponse *res = req->beginResponse_P(200, "text/html; charset=utf-8", INDEX_HTML);
  res->addHeader("Content-Security-Policy", "default-src 'self'; style-src 'unsafe-inline' 'self';");
  req->send(res);
}

void initWeb() {
  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Routes
  server.on("/", HTTP_GET, onIndex);
  server.onNotFound([](AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not found");
  });

  // Start server
  server.begin();
  Serial.println(F("HTTP server (async) started"));
}

// -------------------- Telemetry --------------------
void broadcastState() {
  StaticJsonDocument<256> doc;
  doc["type"]         = "state";
  doc["pageCount"]    = g_pageLoadCount;
  doc["armed"]        = g_armed;
  doc["pulseActive"]  = g_pulseActive;
  doc["wifi"]         = "AP";
  doc["wifiConnected"]= (ws.count() > 0); // proxy for “active client”
  doc["adc"]          = analogRead(PIN_ADC_RESERVED); // reserved input; 0 if floating

  char out[256];
  const size_t n = serializeJson(doc, out, sizeof(out));
  ws.textAll(out, n);
}
