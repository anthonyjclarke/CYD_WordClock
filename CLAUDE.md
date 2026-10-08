# Project: CYD_WordClock

Word clock for ESP32-2432S028R (CYD) displaying time as highlighted words in a 16×14 letter grid on ILI9341 240×320 TFT (portrait). Word logic ported from Brett Oliver's wordclock_v4_9. Version lives only in `FIRMWARE_VERSION` (`include/config.h`); `dev` carries a `-dev` suffix.

## Hardware

- Board: ESP32-2432S028R (CYD), no PSRAM
- Display: ILI9341 240×320 portrait, SPI (TFT_RST=-1)
- Touch: XPT2046 on VSPI (CLK=25, MISO=39, MOSI=32, CS=33)

## Pin Mapping

| Function   | GPIO | Notes                                |
|:-----------|:-----|:-------------------------------------|
| Touch CS   | 33   | Own pins; VSPI shared with display † |
| Touch CLK  | 25   | VSPI                                 |
| Touch MOSI | 32   | VSPI                                 |
| Touch MISO | 39   | VSPI                                 |

† See **Shared SPI bus** under Architecture & Key Gotchas.

## Libraries

- TFT_eSPI (display driver, FreeMonoBold9pt7b font)
- XPT2046_Touchscreen (touch, 130+ ms debounce)
- Adafruit-GFX-Library (fonts, setColorDepth requirement)
- WiFiManager (credential provisioning)
- ezTime (NTP + timezone via POSIX fallback)
- ArduinoJson (WebUI config)

## Architecture & Key Gotchas

**Rendering:** `TFT_eSprite gridSprite` 240×266px, 8-bit depth (63.8 KB). Must call `setColorDepth(8)` before `createSprite()` — 16-bit would be 127 KB (fails without PSRAM). Status strip is separate sprite to avoid per-second flicker. Font: FreeMonoBold9pt7b (grid), FreeSansBold9pt7b (status).

**Word descriptors:** `const byte w_xxx[3] PROGMEM = {row, col, len}` — identical to Brett Oliver's format. Read with `pgm_read_byte()`. `showTimeWords()` is exact port of Brett's logic: exact-minute (not 5-min rounded), MINUTE/MINUTES suffix, time-of-day words (MORNING/AFTERNOON/EVENING/NIGHT on rows 12–13).

**Shared letters:** Grid mirrors Brett's 16×16 physical matrix. TWO/ONE share 'O' at R8 col 2; TWELVE/ELEVEN share 'E' at R6 col 5; EIGHT/FIVE share 'E' at R10 col 8. "A QUARTER" reuses 'A' from HALF (R0 col 13). Do not refactor letter positions.

**Animation:** `computeTimeWords()` populates `gridLit` only (no render) for fade transitions. `animateFade()` runs two-phase crossfade. Type (ANIM_NONE/ANIM_FADE) is runtime-selectable via NVS, no reflash needed.

**Settings (NVS):** All user values stored in Preferences namespace `"wordclock"` as `RuntimeSettings` struct. Fields: `displayFlip`, `brightnessDefault/Min/Max/Steps`, `ldrEnabled/Dark/Bright`, `animType/FadeSteps/FadeMs`, `timezone`, `posixFallback`. Defaults from `config.h`, written on first boot. Use `settings()` (const reference) in application code, not config.h constants directly.

**Touch:** Long press ≥600ms cycles brightness. Steps and range are runtime-configurable.

**Shared SPI bus — latent, not a live bug (checked 11-09-2026).** Display and touch
both run on VSPI: without `-DUSE_HSPI_PORT`, TFT_eSPI 2.5.43 creates `SPIClass(VSPI)`,
and `initTouch()` (`wordclock.cpp`) creates a second one on pins 25/32/39.
`-DTOUCH_CS=33` also makes TFT_eSPI drive GPIO 33 during `tft.init()`. It works
because TFT_eSPI always uses SPI transactions on the ESP32, nothing runs in a second
task, and `tft.init()` runs before `initTouch()` — the bus reads MISO from whichever
`begin()` ran last, which is touch's GPIO 39. It **breaks** if anything reads back
from the panel (`readPixel`, `readRect`, `readcommand`), `tft.init()` is called again
after `initTouch()`, or TFT_eSPI's own `getTouch()` is used.
*Fix:* add `-DUSE_HSPI_PORT` (MOSI 13 / MISO 12 / SCLK 14 / CS 15 are HSPI's native
pins), remove `-DTOUCH_CS=33`, give the touch CS its own name in `config.h`
(`wordclock.cpp` uses `TOUCH_CS` for the constructor and `touchSPI.begin()`), then
test touch on hardware. Checked against arduino-esp32 2.0.17 source, which the
pinned `espressif32@6.12.0` uses — never unpin it (3.x fails to build). Found
while porting CYD_AnimatedPixelClock, where the shared bus turned out **not** to be
the cause of dead touch (a faulty board), so do not treat it as a known failure.

**System actions:** WiFi reset and factory reset are queued via `requestWifiReset()` / `requestFactoryReset()` and executed by `processPendingSystemActions()` in loop — ensures HTTP responses complete before reboot.

**Status strip:** Buffered; skipped if formatted text unchanged. Call `invalidateStatusStrip()` to force repaint (e.g. after display flip).

**Timezone quirk:** `myTZ.setLocation()` HTTP call reliably fails immediately after WiFi connect. Use `NTP_POSIX_FALLBACK` (config.h) — always DST-correct offline. POSIX string cached in NVS.

**Always-on rows:** `showTimeWords()` unconditionally lights THE, TIME, IS (R0).

## WebUI & API

Served on port 80. SPA polls `/api/state` every second for live grid/status preview. Reset endpoints (`/api/reset/settings`, `/api/reset/wifi`, `/api/reset/all`) queue actions safely. Config POST at `/api/config` applies immediately.

## Web installer and releases

Release images come only from `.github/workflows/firmware.yml` on a `v*` tag on `main` (shared cyd-web-installer tooling); never publish a local build.

- **Never put `firmware-merged.bin` in a manifest** — it fills NVS with `0xFF`, so an Update would wipe WiFi and settings.
- **`PROJECT_NAME` and the partition table are frozen** — Update is offered only when Improv's name matches the manifest.
- **Never put Improv back in `lib_deps`** — `lib/ImprovWiFi` is vendored with a parser fix.
- **`improvTick()` must run at least every ~1 s** — keep it in `loop()`, the portal loop and `animateFade()`.
