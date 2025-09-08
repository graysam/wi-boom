# Repository Guidelines

## Project Structure & Module Organization
- Root: Arduino sketch (`hv_trigger_async_ESP8266.ino`), modules (`*.h/*.cpp`).
- Docs: `PROJECT_SPEC.txt` (authoritative behavior), `pin_assignments.txt` (GPIO map).
- Assets: `data/` for LittleFS UI (optional, PlatformIO `uploadfs`).
- Scripts: `scripts/` build/upload (Arduino CLI + PlatformIO).
- Target: ESP8266 (NodeMCU/D1 mini). Pins per `pin_assignments.txt`.

## Build, Test, and Development Commands
- Arduino CLI:
  - `arduino-cli core install esp8266:esp8266`
  - `scripts/build.ps1` or `scripts/build.sh` (compile/upload prompt)
  - OTA: `scripts/ota.ps1` or `scripts/ota.sh` (port 3232)
- PlatformIO: `pio run`, `pio run -t upload`, `pio run -t uploadfs`.
- Quick WS test: `wscat -c ws://10.11.12.1/ws -x '{"cmd":"arm","on":true}'`.

## Coding Style & Naming Conventions
- Language: Arduino/C++.
- Indentation: 2 spaces, no tabs; UTF‑8, LF.
- Naming: `CamelCase` for types, `snake_case` for functions/vars, `UPPER_SNAKE_CASE` for macros; constants like `kTelemetryIntervalMs`.
- Structure: Non‑blocking loops; separate `state`, `fire_engine`, `net`, `storage`.
- Formatting: Keep includes ordered; one module per concern.

## Testing Guidelines
- Behavior is defined in `PROJECT_SPEC.txt` (state machine, timing, WS schema).
- Serial logs at 115200; verify Arm/Fire/Auto‑disarm and LED rules.
- Bench timing with logic analyzer; ensure guard timing for short pulses.
- WS e2e: connect to `ws://10.11.12.1/ws`, send `{"cmd":"cfg", ...}`, confirm telemetry updates every ~250 ms.

## Commit & Pull Request Guidelines
- Conventional Commits: `feat(scope): ...`, `fix(scope): ...`, `docs: ...`, `chore: ...`, `test: ...` (see `git log`).
- Subject ≤ 72 chars; body bullets for key changes and rationale.
- PRs: clear description, link relevant spec sections, screenshots/logs (UI/WS/Serial), tested hardware/ports, any pin/config diffs.

## Security & Configuration Tips
- Do not hard‑code secrets; SoftAP is for local control. Consider STA creds via persisted config.
- Enforce ARMED gating for Fire and reject cfg changes while ARMED.
- Keep network handlers non‑blocking; firing engine must not depend on WS loop.
