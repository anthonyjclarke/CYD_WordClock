# CYD_WordClock

**Repository:** https://github.com/anthonyjclarke/CYD_WordClock

## Project

A word clock for the ESP32 CYD (ESP32-2432S028R). Displays time as highlighted words
in a 16-column × 14-row letter grid on the ILI9341 240×320 TFT in portrait orientation.
Example: "THE TIME IS FOURTEEN MINUTES PAST THREE IN THE AFTERNOON".

Word clock logic is ported from Brett Oliver's Word Clock (wordclock_v4_9, Arduino Nano +
16×16 MAX7219 LED matrix): https://www.brettoliver.org.uk/Word_Clock/Word_Clock.htm
The same `{row, col, length}` PROGMEM word descriptor pattern and `showTimeWords()` logic
are preserved. The LED matrix is replaced by a TFT_eSprite.

Time is exact-minute (not 5-min rounded). MINUTE/MINUTES suffix is shown. Time-of-day
(morning/afternoon/evening/night) is shown on rows 12–13 of the grid.

## Hardware

- Board: ESP32-2432S028R (CYD)
- Display: ILI9341 · 240×320 portrait · SPI
- Touch: XPT2046 · separate SPI bus (VSPI, CS=GPIO33)
- No PSRAM on standard CYD

## Grid Layout

Portrait 240×320. Letter grid: 16 cols × 14 rows, each cell 15×19px = 240×266px total.
Status strip (time HH:MM:SS + date) occupies y=268–320.

The grid matches rows 0–13 of Brett Oliver's physical 16×16 LED matrix (rows 14–15
for temp/seconds are omitted — temp sensor not present on CYD; seconds shown in status strip).

```
R0:  THE TIME IS HALF     — THE, TIME, IS, HALF (A in HALF also used as article "A")
R1:  QUARTERTWENTY        — QUARTER, TWENTY
R2:  TENSIXTEENTWONE      — TEN, SIX, SIXTEEN, TWO, ONE
R3:  EIGHTEENFIVE         — EIGHT, EIGHTEEN, FIVE
R4:  SEVENTEENINETEEN     — SEVEN, SEVENTEEN, NINE, NINETEEN
R5:  FOURTEENTHIRTEEN     — FOUR, FOURTEEN, THIRTEEN
R6:  TWELVELEVENTHREE     — TWELVE, ELEVEN, THREE
R7:  MINUTES PASTO        — MINUTE, S, PAST, TO
R8:  TWONELEVENINESIX     — TWO, ONE, ELEVEN, NINE, SIX  (hours)
R9:  SEVENTHREETWELVE     — SEVEN, THREE, TWELVE  (hours)
R10: FOUR FIVEIGHTEN      — FOUR, FIVE, EIGHT, TEN  (hours)
R11: OCLOCK INAT          — O'CLOCK, IN, AT
R12: NIGHTHE MORNING      — NIGHT, THE (shares T with NIGHT), MORNING
R13: EVENINGAFTERNOON     — EVENING, AFTERNOON, NOON
```

## Architecture

```
src/
  main.cpp       — setup(), loop(), initDisplay/WiFi/Time/Settings/WebUi
  display.cpp    — gridSprite, colour init, pushGrid, brightness, status strip
  wordclock.cpp  — GRID array, PROGMEM word descriptors, clearFrame/addWordToFrame,
                   renderGrid, showTimeWords, touch, wordclockTick
  settings.cpp   — RuntimeSettings NVS persistence (Preferences), defaults from config.h
  runtime.cpp    — applyRuntimeSettings(), processPendingSystemActions(), deferred resets
  webui.cpp      — WebServer port 80, SPA HTML, /api/state, /api/config, /api/reset/*
include/
  config.h       — grid geometry, colours, touch, WiFi, NTP constants, default values
  display.h      — sprite/colour externs, function declarations
  wordclock.h    — gridLit extern, function declarations
  settings.h     — RuntimeSettings struct, settings()/initSettings()/saveSettings() API
  runtime.h      — applyRuntimeSettings(), processPendingSystemActions(), request*Reset()
  webui.h        — initWebUi(), webUiTick()
  debug.h        — leveled debug macros
  secrets.h      — WiFi credentials (gitignored)
```

Rendering: `TFT_eSprite gridSprite` (240×266px, 8-bit depth = 63.8 KB).
Status strip: separate `TFT_eSprite statusSprite` (240×52px, 8-bit depth) to avoid
per-second flicker on the physical panel.
Font: `FreeMonoBold9pt7b` for the main letter grid, `FreeSansBold9pt7b` for the
status strip (Adafruit GFX free fonts, available via `-DLOAD_GFXFF=1`).
Each cell draws one letter centred with `MC_DATUM`. Lit cells use `colourLit` (warm white);
dim cells use `colourDim` (dark grey).

## Runtime Settings (NVS)

All user-tuneable values are stored in NVS (`Preferences` namespace `"wordclock"`) as a
`RuntimeSettings` struct. `config.h` constants supply the compiled-in defaults — they are
written to NVS on first boot and whenever settings are reset.

| Field               | NVS key      | Default source          |
|:--------------------|:-------------|:------------------------|
| `displayFlip`       | `disp_flip`  | `DISPLAY_FLIP`          |
| `brightnessDefault` | `bri_def`    | `BRIGHTNESS_DEFAULT`    |
| `brightnessMin/Max` | `bri_min/max`| `BRIGHTNESS_MIN/MAX`    |
| `brightnessSteps`   | `bri_step`   | `BRIGHTNESS_STEPS`      |
| `ldrEnabled`        | `ldr_en`     | `LDR_ENABLED`           |
| `ldrDark/Bright`    | `ldr_dark/bright` | `LDR_DARK/BRIGHT`  |
| `animType`          | `anim_type`  | `ANIM_TYPE`             |
| `animFadeSteps`     | `fade_steps` | `ANIM_FADE_STEPS`       |
| `animFadeMs`        | `fade_ms`    | `ANIM_FADE_MS`          |
| `timezone`          | `tz_name`    | `NTP_TIMEZONE`          |
| `posixFallback`     | `tz_posix`   | `NTP_POSIX_FALLBACK`    |

## Web UI

Served by `WebServer` on port 80. Access at `http://<device-ip>/`.

| Endpoint               | Method | Purpose                                      |
|:-----------------------|:-------|:---------------------------------------------|
| `/`                    | GET    | Single-page app — live clock + config        |
| `/api/state`           | GET    | JSON: time, date, grid rows+lit, system info |
| `/api/config`          | GET    | JSON: all `RuntimeSettings` fields           |
| `/api/config`          | POST   | Update settings; applies immediately         |
| `/api/reset/settings`  | POST   | Reset settings to `config.h` defaults        |
| `/api/reset/wifi`      | POST   | Clear WiFiManager credentials, reboot        |
| `/api/reset/all`       | POST   | Factory reset (clear NVS + WiFi), reboot     |

The SPA polls `/api/state` every second to animate the live grid preview and status strip.
System resets are queued via `requestWifiReset()` / `requestFactoryReset()` and executed
safely by `processPendingSystemActions()` in `loop()` after the HTTP response is sent.

## Key Architecture Notes

- Word descriptors: `const byte w_xxx[3] PROGMEM = { row, col, len }` — identical to
  Brett Oliver's format. `addWordToFrame()` reads with `pgm_read_byte()`.
- `showTimeWords()`: exact port of Brett's logic. Individual minutes (not 5-min rounding).
  TWENTY + units for 21–29, HALF for 30, etc. MINUTE/MINUTES suffix, time-of-day words.
- `computeTimeWords()` (internal): populates `gridLit` only, without rendering — used by
  the animation path to compute the new frame before starting the fade transition.
- Shared letters: TWO/ONE share 'O' at R8 col 2; TWELVE/ELEVEN share 'E' at R6 col 5;
  EIGHT/FIVE share 'E' at R10 col 8, etc. This mirrors the physical LED matrix design.
- "A QUARTER": lights the 'A' at R0 col 13, which is the A in HALF. Faithful to original.
- Sprite `setColorDepth(8)` required — 16-bit at 240×266 = 127 KB (fails on CYD without PSRAM).
- Status strip redraws are buffered and skipped when the formatted text has not changed.
  Call `invalidateStatusStrip()` to force a repaint (e.g. after display flip).
- Touch: VSPI bus (CLK=25, MISO=39, MOSI=32, CS=33). Long press ≥600ms cycles brightness.
  Brightness steps and range are runtime-configurable via `settings().brightnessSteps/Min/Max`.
- Animation type (`ANIM_NONE` / `ANIM_FADE`) is runtime-selectable — no reflash needed.
  `animateFade()` runs a two-phase crossfade: fade out departing words, then fade in arriving words.
- `settings()` returns a const reference to the global `RuntimeSettings` — always use this
  for runtime values instead of reading `config.h` constants directly in application code.
- System actions (WiFi reset, factory reset) are queued via `request*Reset()` and executed
  by `processPendingSystemActions()` in `loop()` to ensure HTTP responses complete first.

## Known Issues / Quirks

- `TFT_RST=-1` is correct — no reset pin on CYD.
- `myTZ.setLocation()` HTTP call reliably fails immediately after WiFi connect.
  `NTP_POSIX_FALLBACK` in config.h handles this — always DST-correct offline.
- Sprite `setColorDepth(8)` must be called before `createSprite()`. See heap note above.
- `showTimeWords()` always lights THE, TIME, IS. These are always on (row 0, always lit).

## Flashing Notes

- Port: `/dev/cu.usbserial-*` (CP2102)
- Upload speed: 230400 (reduce to 115200 if errors)
- Serial log sequence: "Display initialised" → "WiFi OK" → "Time synced" →
  "[WordClock] HH:MM" on each minute change.

## Rules

- Global rules: `~/.claude/CLAUDE.md`
- Type rules: `~/.claude/rules/cyd-esp32.md`
