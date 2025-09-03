# WebSocket API (ws://10.11.12.1/ws)

This device exposes a single WebSocket endpoint at `/ws`. Messages are newline‑free JSON objects. There are no explicit acks; the device pushes a fresh `state` message after handling a command and on a periodic tick (~250 ms).

Conventions
- Numbers are integers (ms, counts). Unknown fields are ignored.
- While `armed=true`, config (`cfg`) changes are rejected.
- UI enables FIRE only when `armed=true`.

State Telemetry
Sent periodically and after commands:

{
  "type": "state",
  "armed": false,
  "pulseActive": false,
  "pageCount": 1,
  "wifiClients": 0,
  "wifiConnected": false,
  "adc": 0,
  "cfg": { "mode": "single", "width": 10, "spacing": 20, "repeat": 1 }
}

Commands
1) Arm/Disarm
{ "cmd": "arm", "on": true }

2) Configure (ignored when armed=true)
{ "cmd": "cfg", "mode": "single|buzz", "width": 5..100, "spacing": 10..100, "repeat": 1..4 }

3) Fire (only when armed=true and not already firing)
{ "cmd": "fire" }

Example Flows
- Configure while disarmed, then arm and fire:
  - send {"cmd":"cfg","mode":"buzz","width":12,"spacing":25,"repeat":2}
  - wait for state with updated cfg
  - send {"cmd":"arm","on":true}
  - wait for state armed=true, then send {"cmd":"fire"}

Testing Tools
- Windows PowerShell: `.tools/ws_test.ps1`
- macOS/Linux: `npx wscat -c ws://10.11.12.1/ws`

Notes
- After firing completes, the device auto‑disarms.
- `wifiConnected=true` means at least one WS client connected; the green LED is solid in this state.
