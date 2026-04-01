#pragma once
// config.h — CYD WordClock user-tuneable constants

// ── Firmware version ──────────────────────────────────────────────────────────
#define FW_VERSION          "1.1"

// ── Display ───────────────────────────────────────────────────────────────────
#define DISPLAY_FLIP        1           // 1 = flip portrait 180° (USB connector at top)
#define SCREEN_ROTATION     (DISPLAY_FLIP ? 2 : 0)  // portrait: 0 = USB at bottom, 2 = USB at top
#define BRIGHTNESS_DEFAULT  180         // 0–255 backlight PWM

// ── Word Grid Geometry ────────────────────────────────────────────────────────
// 16-column layout matches Brett Oliver wordclock_v4_9 physical matrix
// 14 rows covers the word rows (0–13); temp/seconds rows omitted
#define GRID_COLS           16          // letters per row
#define GRID_ROWS           14          // rows (matches reference rows 0–13)
#define CELL_W              15          // px per cell wide  (16×15 = 240 — exact fit)
#define CELL_H              19          // px per cell tall  (14×19 = 266px)
#define GRID_HEIGHT         (GRID_ROWS * CELL_H)  // 266
#define STATUS_Y            (GRID_HEIGHT + 2)      // 268 — top of status strip
#define STATUS_HEIGHT       (320 - STATUS_Y)       // remaining px for status strip

// ── Colours ───────────────────────────────────────────────────────────────────
// Lit word — warm white
#define COLOUR_LIT_R        255
#define COLOUR_LIT_G        255
#define COLOUR_LIT_B        210
// Dim letter — very dark grey (visible but not distracting)
#define COLOUR_DIM_R        35
#define COLOUR_DIM_G        35
#define COLOUR_DIM_B        35
// Grid background
#define COLOUR_BG_R         8
#define COLOUR_BG_G         8
#define COLOUR_BG_B         8
// Status strip text
#define COLOUR_STATUS_R     120
#define COLOUR_STATUS_G     120
#define COLOUR_STATUS_B     120

// ── Touch ─────────────────────────────────────────────────────────────────────
#define TOUCH_SCLK          25          // VSPI CLK
#define TOUCH_MISO          39          // VSPI MISO (input-only GPIO)
#define TOUCH_MOSI          32          // VSPI MOSI
// TOUCH_CS = 33 set via build flag -DTOUCH_CS=33

#define TOUCH_PRESSURE_MIN  200
#define TOUCH_LONG_PRESS_MS 600         // ms held to trigger long-press (brightness)
#define TOUCH_DEBOUNCE_MS   300
#define BRIGHTNESS_STEPS    4

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define WIFI_AP_NAME        "CYD-WordClock"
#define WIFI_TIMEOUT_S      60

// ── NTP / Time ────────────────────────────────────────────────────────────────
#define NTP_TIMEZONE        "Australia/Sydney"
// POSIX fallback: offline DST-aware rule for Australia/Sydney
#define NTP_POSIX_FALLBACK  "AEST-10AEDT,M10.1.0,M4.1.0/3"
#define NTP_SYNC_TIMEOUT_S  20

// ── Auto-brightness (LDR) ─────────────────────────────────────────────────────
#define LDR_ENABLED         0           // 1 = auto-brightness; 0 = long-press only
#define LDR_PIN             34
#define LDR_DARK            200
#define LDR_BRIGHT          2500
#define BRIGHTNESS_MIN      15
#define BRIGHTNESS_MAX      255
#define LDR_SAMPLES         8
#define LDR_UPDATE_MS       5000

// ── Animations ────────────────────────────────────────────────────────────────
#define ANIM_NONE           0           // instant update, no animation
#define ANIM_FADE           1           // fade out old words then fade in new words
#define ANIM_TYPE           ANIM_FADE   // select animation style
#define ANIM_FADE_STEPS     14          // steps per fade phase (0→steps inclusive)
#define ANIM_FADE_MS        18          // ms per step (~270ms per phase, ~540ms total)

// ── Debug ─────────────────────────────────────────────────────────────────────
#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL 3
#endif
