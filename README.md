# CYD Word Clock

A word clock for the ESP32-2432S028R (CYD). Displays time as highlighted words in a
16×14 letter grid: "THE TIME IS FOURTEEN MINUTES PAST THREE IN THE AFTERNOON".

Ported from [Brett Oliver's Word Clock](https://www.brettoliver.org.uk/Word_Clock/Word_Clock.htm)
(wordclock_v4_9, Arduino Nano + 16×16 MAX7219 LED matrix).
Same grid layout and word logic — adapted for the CYD's ILI9341 TFT display.

## Hardware

ESP32-2432S028R (Cheap Yellow Display) — ILI9341 240×320, XPT2046 touch, LDR.

## Setup

1. Copy `include/secrets.h` and fill in your WiFi credentials.
2. Flash via PlatformIO (`pio run -t upload`).
3. On first boot, connect to the `CYD-WordClock` AP and configure WiFi.
4. Time syncs via NTP automatically. Timezone: Australia/Sydney.

## Display

Portrait 240×320. 16-column × 14-row letter grid occupying the upper 266px.
A status strip at the bottom shows HH:MM:SS and the date.

Exact-minute time phrases — e.g. "THE TIME IS THREE MINUTES PAST TWO IN THE AFTERNOON".

Touch and hold anywhere for ≥600ms to cycle brightness (4 steps).

## Changing Timezone

Edit `NTP_TIMEZONE` and `NTP_POSIX_FALLBACK` in `include/config.h`.
