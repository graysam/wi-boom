#include "net.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

extern "C" {
#include "user_interface.h" // wifi_softap_get_station_num
}

#include "config.h"
#include "pins.h"
#include "state.h"
#include "fire_engine.h"
#include "storage.h"
#include "web_ui.h"

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static volatile uint16_t g_ws_clients = 0;

static void handle_ws_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    g_ws_clients = ws.count();
    state_set_ws_connected(g_ws_clients > 0);
    Serial.printf("WS: connect #%u, total=%u\n", client->id(), g_ws_clients);
  } else if (type == WS_EVT_DISCONNECT) {
    g_ws_clients = ws.count();
    state_set_ws_connected(g_ws_clients > 0);
    Serial.printf("WS: disconnect #%u, total=%u\n", client->id(), g_ws_clients);
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) return;
    // Parse JSON command
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) { Serial.printf("WS: parse error: %s\n", err.c_str()); return; }

    const char* cmd = doc["cmd"] | "";
    if (strcmp(cmd, "arm") == 0) {
      bool on = doc["on"].as<bool>();
      if (state_arm(on)) Serial.printf("ARM %s\n", on?"on":"off");
    } else if (strcmp(cmd, "cfg") == 0) {
      Config c = state_get().cfg;
      if (const char* m = doc["mode"]) {
        c.mode = (strcmp(m, "buzz") == 0) ? FireMode::Buzz : FireMode::Single;
      }
      if (doc.containsKey("width")) c.width_ms = constrain((int)doc["width"], 5, 100);
      if (doc.containsKey("spacing")) c.spacing_ms = constrain((int)doc["spacing"], 10, 100);
      if (doc.containsKey("repeat")) c.repeat = constrain((int)doc["repeat"], 1, 4);
      if (state_set_cfg(c)) {
        storage_save_cfg(c);
        Serial.printf("CFG mode=%s width=%u spacing=%u repeat=%u\n", c.mode==FireMode::Single?"single":"buzz", c.width_ms, c.spacing_ms, c.repeat);
      }
    } else if (strcmp(cmd, "fire") == 0) {
      if (fire_engine_trigger()) Serial.println("FIRE start"); else Serial.println("FIRE rejected");
    }
    net_broadcast_telemetry();
  }
}

void net_setup() {
  // LEDs: WAIT amber while network not ready
  digitalWrite(PIN_LED_WAIT_AMBER, HIGH);

  WiFi.mode(WIFI_AP_STA);
  IPAddress apIP(SOFTAP_IP_OCT1, SOFTAP_IP_OCT2, SOFTAP_IP_OCT3, SOFTAP_IP_OCT4);
  IPAddress gw(apIP);
  IPAddress mask(255,255,255,0);
  WiFi.softAPConfig(apIP, gw, mask);
  WiFi.softAP(SOFTAP_SSID, SOFTAP_PASS);
  Serial.print(F("SoftAP IP: ")); Serial.println(apIP);

  bool served_fs = false;
  if (LittleFS.begin()) {
    if (LittleFS.exists("/index.html")) {
      server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
      served_fs = true;
      Serial.println(F("Serving UI from LittleFS"));
    }
  }

  if (!served_fs) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
      req->send_P(200, "text/html", INDEX_HTML);
      auto& S = const_cast<DeviceState&>(state_get());
      S.page_count++;
    });
  }

  ws.onEvent(handle_ws_event);
  server.addHandler(&ws);

  server.begin();

  digitalWrite(PIN_LED_WAIT_AMBER, LOW);
}

void net_broadcast_telemetry() {
  const auto& S = state_get();
  // Update LED_READY based on WS connection
  digitalWrite(PIN_LED_READY_GREEN, S.ws_connected ? HIGH : LOW);
  // WAIT amber when no WS client connected
  digitalWrite(PIN_LED_WAIT_AMBER, S.ws_connected ? LOW : HIGH);
  // Armed LED: slow blink while armed
  static uint32_t last = 0; static bool blink = false;
  if (S.armed && millis() - last > 500) { last = millis(); blink = !blink; }
  if (!S.armed) blink = false;
  digitalWrite(PIN_LED_ARMED_RED, (S.armed && blink) ? HIGH : LOW);

  StaticJsonDocument<384> doc;
  doc["type"] = "state";
  doc["armed"] = S.armed;
  doc["pulseActive"] = S.firing; // reflects engine state
  doc["pageCount"] = S.page_count;
  // SoftAP station count
  uint8_t stations = wifi_softap_get_station_num();
  doc["wifiClients"] = stations;
  doc["wifiConnected"] = (g_ws_clients > 0);
  doc["staConnected"] = S.sta_connected;
  doc["staIP"] = S.sta_ip.toString();
  doc["adc"] = 0;
  JsonObject cfg = doc.createNestedObject("cfg");
  cfg["mode"] = (S.cfg.mode == FireMode::Single) ? "single" : "buzz";
  cfg["width"] = S.cfg.width_ms;
  cfg["spacing"] = S.cfg.spacing_ms;
  cfg["repeat"] = S.cfg.repeat;

  char buf[512];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  ws.textAll(buf, n);
}
