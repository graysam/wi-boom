// ============================================================================
// file: web_server.h
// Async web server, websocket, routes, and actions.
// ============================================================================

#pragma once
#include <Arduino.h>

void setupWiFiAP();
void initWeb();
void broadcastState();
void updateIndicators();

// Settings
void setAllowUnsupervisedTimer(bool allow);
bool getAllowUnsupervisedTimer();

// Network settings (runtime-configurable)
void setStaConfig(bool enabled, const String &ssid, const String &pass);
void getStaConfig(bool &enabled, String &ssid);

// Actions that UI may invoke
bool actionArm(bool enabled);
bool actionFire();
