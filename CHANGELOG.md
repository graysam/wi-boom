# Changelog

All notable changes to this project are documented here. Versions follow `MAJOR.MINOR.PATCH`.

## v0.1.2
- feat(ui): Status bar rework with WS/Armed LEDs, compact W/S/R values, and AP name.
- feat(ui): Auto‑reconnect WebSocket with overlay veil while disconnected.
- feat(ui): Live slider value boxes with units; Arm/Disarm buttons toggle visually.
- fix(ui): Content‑Security‑Policy allows inline script and ws/wss connect.
- feat: Add `tools/test_ws.ps1` WebSocket regression test (two‑client sync, arm/cfg/fire).
- docs: Update README, build notes, and WS_API with new fields (`apSSID`, `wsCount`) and OTA instructions.

## v0.1.1
- feat(esp32): Add ESP32 Dev Module (ESP32‑WROOM‑32) pin map; keep ESP32‑S3 compatibility.
- docs: Update build scripts and notes for both ESP32 and ESP32‑S3.
- fix(ui): Initial CSP header tweak to support inline UI.

