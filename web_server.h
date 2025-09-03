// ============================================================================
// file: web_server.h
// Async web server, websocket, routes, and actions.
// ============================================================================

#pragma once
#include <Arduino.h>

void setupWiFiAP();
void initWeb();
void broadcastState();

// Actions that UI may invoke
bool actionArm(bool enabled);
bool actionSetIndicator(const String &which, bool on);
bool actionFire();
