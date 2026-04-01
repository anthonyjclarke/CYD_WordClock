# Changelog

## [1.1] 2026-04-01

### Added
- **Web UI** (`src/webui.cpp`, `include/webui.h`) — ESP32 `WebServer` on port 80
  - `/` — Single-page app: live word-grid preview (1× / 2× scale toggle), config form, system info
  - `/api/state` — JSON: current time, date, full 16×14 grid (text + lit bitmask), system info
  - `/api/config` GET/POST — read and write all runtime settings; changes apply immediately
  - `/api/reset/settings`, `/api/reset/wifi`, `/api/reset/all` — maintenance actions
  - `webUiTick()` called from `loop()` to service HTTP requests
- **Runtime settings** (`src/settings.cpp`, `include/settings.h`) — NVS-backed `RuntimeSettings` struct
  - Persists display flip, brightness (default/min/max/steps), LDR, animation type/speed, timezone
  - `initSettings()` loads from NVS on boot; defaults come from `config.h` constants
  - `saveSettings()` validates and clamps before writing; `resetSettingsToDefaults()` / `clearStoredSettings()` for maintenance
- **Runtime coordinator** (`src/runtime.cpp`, `include/runtime.h`)
  - `applyRuntimeSettings()` — pushes live settings to display rotation, touch rotation, brightness, timezone in one call
  - `processPendingSystemActions()` — deferred WiFi reset, settings reset, factory reset (safe to call from `loop()`)
- Animation system: `ANIM_TYPE` in `config.h` selects default transition style
  - `ANIM_NONE` — instant update (original behaviour)
  - `ANIM_FADE` — fade out departing words then fade in arriving words (runtime-selectable via Web UI)
- `ANIM_FADE_STEPS` / `ANIM_FADE_MS` tune fade speed (~540ms total at defaults)
- `DISPLAY_FLIP` in `config.h` — set to `1` to rotate portrait display 180° (USB at top); also settable via Web UI
- `getGridRowText(row)` — exposes static GRID array rows to external renderers (used by Web UI `/api/state`)
- `redrawClockNow()` — forces immediate full redraw of grid and status strip (used by `applyRuntimeSettings()`)
- `updateTouchRotation(rotation)` — updates XPT2046 rotation to match display after runtime flip change
- `invalidateStatusStrip()` — forces status strip repaint on next tick (used after display flip)

### Changed
- `showTimeWords()` refactored: word-compute logic extracted to internal
  `computeTimeWords()` so the animation path can populate `gridLit` without
  triggering an immediate render
- Animation type check converted from compile-time `#if ANIM_TYPE` to runtime
  `if (settings().animType == ANIM_FADE)` — type is now changeable via Web UI without reflash
- LDR auto-brightness parameters (`ldrDark`, `ldrBright`, `brightnessMin`, `brightnessMax`) read from
  `settings()` at runtime rather than `config.h` compile-time constants; toggle via Web UI
- Touch brightness cycling uses `settings().brightnessMin/Max/Steps` instead of hardcoded 4-level array
- Display rotation and initial brightness read from `settings()` on boot instead of `config.h` constants
- `initTime()` reads timezone/POSIX fallback from `settings()` rather than `F(NTP_TIMEZONE)` literal
- Bottom status strip now renders through a dedicated sprite, eliminating the
  visible flicker caused by direct `fillRect()` + redraw on the TFT each second
- Main word grid font changed from `FreeSansBold9pt7b` to `FreeMonoBold9pt7b`
  for more even visual spacing across the fixed 16-column layout
- `lastTimeBuf` / `lastDateBuf` status-strip dedup buffers moved to file scope so
  `invalidateStatusStrip()` can reset them from outside `drawStatusStrip()`

---

## [1.0] 2026-03-19

### Added
- Initial release: word clock on CYD ILI9341 240×320 portrait display
- 16×14 letter grid ported from Brett Oliver's Word Clock wordclock_v4_9 (Arduino/MAX7219)
  — https://www.brettoliver.org.uk/Word_Clock/Word_Clock.htm
- Same `{row, col, length}` PROGMEM word descriptor structure as reference
- Same `showTimeWords()` logic: exact-minute display (not 5-min rounding)
- MINUTE/MINUTES suffix, time-of-day context (morning/afternoon/evening/night)
- WiFi provisioning via tzapu/WiFiManager
- NTP time sync via ezTime — Australia/Sydney timezone with POSIX DST fallback
- XPT2046 touch: long-press brightness cycling (4 steps)
- LDR auto-brightness support (off by default, enable via LDR_ENABLED in config.h)
- TFT_eSprite 240×266 grid buffer (8-bit depth) for flicker-free updates
- Status strip showing HH:MM:SS and date (DOW DD MON YYYY)
- FreeSansBold9pt7b GFX font for letter grid rendering
