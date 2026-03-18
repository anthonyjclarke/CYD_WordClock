# Changelog

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
