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

// Actions that UI may invoke
bool actionArm(bool enabled);
bool actionFire();
