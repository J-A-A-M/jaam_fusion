# CLAUDE.md

File guides Claude Code (claude.ai/code) for this repo.

## Project Overview

JAAM Fusion = ESP32 firmware. Shows real-time Ukrainian air alert data, weather, other info via WS2812B LEDs in Ukraine map shape. Targets ESP32, ESP32-S3, ESP32-C3.

## Build Commands

```bash
# Build default firmware (ESP32)
platformio run

# Build and upload
platformio run --target upload

# Build specific target
platformio run -e firmware_esp32s3
platformio run -e firmware_esp32c3
platformio run -e telnet        # includes TelnetSpy for debugging

# Serial monitor
platformio device monitor

# Test environments
platformio run -e test_animation
platformio run -e test_min_of_silence
```

Available environments: `firmware`, `firmware_esp32s3`, `firmware_esp32c3`, `firmware_esp32s3_matrix`, `telnet`, `test_animation`, `test_min_of_silence`, `updater`.

## Pre-build Pipeline

Before every build, PlatformIO runs `tools/pre_build.py` — two scripts in order:
1. `tools/compress_assets.py` — GZIP-compresses `web/*.css`, `web/*.js`, `web/*.json` into `src/web_assets.h` (auto-generated, do not edit)
2. `tools/convert_region_map.py` — converts `data/region_maps.json` into C++ lookup tables in `src/JaamConfig_Generated.h` (auto-generated, do not edit)

**Edit web assets in `web/` only.** `src/web_assets.h` regenerated each build.

## Architecture

### Threading Model
- **Core 0**: WiFi, WebSocket, HTTP web server (ESP-IDF / Arduino WiFi stack)
- **Core 1**: `loop()` — `async.run()` dispatches all timed tasks; `animation.update()` called via `async.setInterval`
- Two FreeRTOS mutexes protect LED state: `animMutex` (animation data) and `stripMutex` (strip hardware writes). `strip->show()` blocks 18–30 ms holding `stripMutex`.

### Key Modules

| File | Role |
|------|------|
| `JaamFusion.cpp` | Entry point (`setup`/`loop`), WebSocket packet parsing, all `handle*` and `request*` dispatch functions |
| `JaamAnimation.cpp/.h` | `AnimationManager` — LED state machine, `update()` renders all strips each tick |
| `JaamConfig.h` | All constants, namespaces (`AlertModes`, `MapModes`, `AnimationTypes`), `enum Type` for every settings key |
| `JaamConfig_Generated.h` | Auto-generated LED lookup tables (`ledLookupKeys/Start/Count/Regions`) for O(log N) region→LED mapping |
| `JaamSettings.cpp/.h` | `JaamSettings` — wraps `Preferences` (NVS), typed get/save for all `Type` enum keys |
| `JaamWeb.cpp/.h` | HTTP endpoints (port 8080), serves compressed static assets, dynamic JSON/HTML with runtime GZIP |
| `JaamApi.cpp/.h` | WebSocket client to `jaam.net.ua`, binary packet parsing |
| `JaamUtils.h` | Utility functions, LED lookup helpers, binary search over sorted `customMap` |
| `JaamLed.cpp/.h` | Low-level LED strip management, `customMap` (sorted by `region_id`) |
| `JaamStorage.cpp/.h` | SPIFFS file I/O, `/custom_map.json` persistence |
| `JaamSound.cpp/.h` | Buzzer + DFPlayer PRO/Mini sound backend, runtime-selected by `SOUND_SOURCE` |

Other hardware modules (one per peripheral, no cross-dependencies): `JaamHardware`, `JaamDisplay`, `JaamButton`, `JaamSiren`, `JaamBattery`, `JaamClimateSensor`, `JaamLightSensor`, `JaamMDNS`, `JaamWifi`, `JaamLogs`, `JaamFirmwareUpdate`.

### DFPlayer PRO/Mini Runtime Backend

Only one physical audio module is ever wired up, but firmware doesn't know which at compile time — `JaamSound.h` resolves it at runtime via `DFBackend::NONE/PRO/MINI` (`namespace DFBackend` in `JaamSound.h`). `initDFPlayer(backend)` probes and retries `begin()` up to 5x/1s apart — shared across PRO/Mini since their `begin()` differs. `getDFBackend()` returns whichever is actually connected; `isDFPlayerEnabled()` must not report stale PRO/MINI if pins are removed or source switches away from DF.

### Animation System (LedState Table)

`AnimationManager` uses two flat arrays, no heap-allocated objects:
- `LedState mainStates[500]` — one entry per physical LED position on `strip_main`
- `LedState serviceStates[N]` — service indicator strip
- `StripState bgState` — single animation for entire `strip_bg`
- `StripState mainOverride` — for `RUNNING_LIGHT` / `SET_BRIGHTNESS` modes

`createAnimation(...)` signature must stay stable — `JaamFusion.cpp` calls it without knowing internal layout. `update()` loops dirty states, acquires `stripMutex` per strip, calls `strip->show()` once per strip per tick.

### Region ID vs. LED Position

`region_id` = arbitrary integer (0–9999), **not** sequential index. `MAX_REGIONS = 169` = entry count in LED lookup table. Always use binary search helpers in `JaamUtils.h` / `JaamLed.cpp` to map `region_id → LED positions`. `alertsFlat[MAX_REGIONS]` indexed by position in `customMap`, not by `region_id` directly.

### WebSocket Binary Protocol

Packets from `jaam.net.ua`:
- Byte 0: type (`0xA1` alerts, `0xA2` notifications, `0xA3` weather, `0xA6`/`0xA7` firmware)
- Alert records: `2B region_id + 2B flags16` (or `1B flags8` compact)
- Trailing 4-byte hash: `2B actual + 2B prev`

### Settings

All settings use `enum Type` (defined in `JaamConfig.h`). Access via `settings.getInt(Type::FOO)` / `settings.saveInt(Type::FOO, value)`. Persisted to NVS via `Preferences`. Web UI reads/writes via HTTP endpoints in `JaamWeb.cpp`.

### UI Controls Architecture

Web UI uses split schema:
- `web/controls.json` → `/ui-schema/controls` (static structure, ETag-cached)
- `JaamWeb.cpp:buildUiSchemaControlsValues()` → `/ui-schema/controls/values` (dynamic current values)
- Client merges both via `mergeControlsWithValues()` in `web/scripts.js`

Adding new setting: update all three — `enum Type` in `JaamConfig.h`, entry in `web/controls.json`, handling in `JaamWeb.cpp`.

## Language

All communication with repo owner: Ukrainian.

## graphify

Project knowledge graph at `graphify-out/` — god nodes, community structure, cross-file relationships.

Rules:
- ALWAYS read `graphify-out/GRAPH_REPORT.md` before reading source files, running grep/glob, or answering codebase questions. Graph = primary map.
- IF `graphify-out/wiki/index.md` EXISTS, navigate it instead of reading raw files
- Cross-module "how does X relate to Y" questions: prefer `graphify query "<question>"`, `graphify path "<A>" "<B>"`, or `graphify explain "<concept>"` over grep — traverses graph's EXTRACTED + INFERRED edges
- After modifying code, run `graphify update .` to keep graph current (AST-only, no API cost).