#include "wordclock.h"
#include "display.h"
#include "config.h"
#include "debug.h"

#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <ezTime.h>

// ── Letter grid ───────────────────────────────────────────────────────────────
// 16-column × 14-row grid ported from Brett Oliver wordclock_v4_9 physical matrix.
// Row/col/len word positions are preserved exactly. Shared letters (e.g. TWO/ONE
// sharing 'O' at row 8 col 2) are preserved as in the original.
//
//                0123456789012345
static const char GRID[GRID_ROWS][GRID_COLS + 1] = {
  "THE TIME IS HALF",  // R0:  THE(0,3) TIME(4,4) IS(9,2) HALF(12,4)
  "QUARTERTWENTY   ",  // R1:  QUARTER(0,7) TWENTY(7,6)
  "TENSIXTEENTWONE ",  // R2:  TEN(0,3) SIX(3,3) SIXTEEN(3,7) TWO(10,3) ONE(12,3)
  "EIGHTEENFIVE    ",  // R3:  EIGHT(0,5) EIGHTEEN(0,8) FIVE(8,4)
  "SEVENTEENINETEEN",  // R4:  SEVEN(0,5) SEVENTEEN(0,9) NINE(8,4) NINETEEN(8,8)
  "FOURTEENTHIRTEEN",  // R5:  FOUR(0,4) FOURTEEN(0,8) THIRTEEN(8,8)
  "TWELVELEVENTHREE",  // R6:  TWELVE(0,6) ELEVEN(5,6) THREE(11,5)
  "MINUTES PASTO   ",  // R7:  MINUTE(0,6) S(6,1) PAST(8,4) TO(11,2)
  "TWONELEVENINESIX",  // R8:  TWO(0,3) ONE(2,3) ELEVEN(4,6) NINE(9,4) SIX(13,3)
  "SEVENTHREETWELVE",  // R9:  SEVEN(0,5) THREE(5,5) TWELVE(10,6)
  "FOUR FIVEIGHTEN ",  // R10: FOUR(0,4) FIVE(5,4) EIGHT(8,5) TEN(12,3)
  "OCLOCK INAT     ",  // R11: OCLOCK(0,6) IN(7,2) AT(9,2)
  "NIGHTHE MORNING ",  // R12: NIGHT(0,5) THE(4,3) MORNING(8,7)
  "EVENINGAFTERNOON",  // R13: EVENING(0,7) AFTERNOON(7,9) NOON(12,4)
};

// ── Frame buffer ──────────────────────────────────────────────────────────────
bool gridLit[GRID_ROWS][GRID_COLS];

// ── Word descriptors: { row, col, len } in PROGMEM ───────────────────────────
// Ported verbatim from Brett Oliver's time.h

// Row 0
static const byte w_the[3]      PROGMEM = {  0,  0, 3 };  // THE
static const byte w_time[3]     PROGMEM = {  0,  4, 4 };  // TIME
static const byte w_is[3]       PROGMEM = {  0,  9, 2 };  // IS
static const byte w_half[3]     PROGMEM = {  0, 12, 4 };  // HALF
static const byte w_a[3]        PROGMEM = {  0, 13, 1 };  // A (article — shares 'A' in HALF)

// Row 7
static const byte w_minute[3]   PROGMEM = {  7,  0, 6 };  // MINUTE
static const byte w_s[3]        PROGMEM = {  7,  6, 1 };  // S (plural)
static const byte w_past[3]     PROGMEM = {  7,  8, 4 };  // PAST
static const byte w_to[3]       PROGMEM = {  7, 11, 2 };  // TO

// Row 11
static const byte w_oclock[3]   PROGMEM = { 11,  0, 6 };  // O'CLOCK
static const byte w_in[3]       PROGMEM = { 11,  7, 2 };  // IN
static const byte w_at[3]       PROGMEM = { 11,  9, 2 };  // AT

// Rows 12–13 (time-of-day)
static const byte w_night[3]    PROGMEM = { 12,  0, 5 };  // NIGHT
static const byte w_the_tod[3]  PROGMEM = { 12,  4, 3 };  // THE (shares T with NIGHT)
static const byte w_morning[3]  PROGMEM = { 12,  8, 7 };  // MORNING
static const byte w_evening[3]  PROGMEM = { 13,  0, 7 };  // EVENING
static const byte w_afternoon[3]PROGMEM = { 13,  7, 9 };  // AFTERNOON
static const byte w_noon[3]     PROGMEM = { 13, 12, 4 };  // NOON (within AFTERNOON)

// Minutes words [0]=one … [19]=twenty
// Index matches (minute - 1) for m=1–20, and is reused for "to" calculations.
// [14] = QUARTER (used for m=15 and m=45 — 1/4 hour).
static const byte w_minutes[20][3] PROGMEM = {
  {  2, 12, 3 },  // [0]  one
  {  2, 10, 3 },  // [1]  two
  {  6, 11, 5 },  // [2]  three
  {  5,  0, 4 },  // [3]  four
  {  3,  8, 4 },  // [4]  five
  {  2,  3, 3 },  // [5]  six
  {  4,  0, 5 },  // [6]  seven
  {  3,  0, 5 },  // [7]  eight
  {  4,  8, 4 },  // [8]  nine
  {  2,  0, 3 },  // [9]  ten
  {  6,  5, 6 },  // [10] eleven
  {  6,  0, 6 },  // [11] twelve
  {  5,  8, 8 },  // [12] thirteen
  {  5,  0, 8 },  // [13] fourteen
  {  1,  0, 7 },  // [14] quarter (QUARTER on row 1)
  {  2,  3, 7 },  // [15] sixteen
  {  4,  0, 9 },  // [16] seventeen
  {  3,  0, 8 },  // [17] eighteen
  {  4,  8, 8 },  // [18] nineteen
  {  1,  7, 6 },  // [19] twenty
};

// Hour words [0]=one … [11]=twelve
static const byte w_hours[12][3] PROGMEM = {
  {  8,  2, 3 },  // [0]  one
  {  8,  0, 3 },  // [1]  two
  {  9,  5, 5 },  // [2]  three
  { 10,  0, 4 },  // [3]  four
  { 10,  5, 4 },  // [4]  five
  {  8, 13, 3 },  // [5]  six
  {  9,  0, 5 },  // [6]  seven
  { 10,  8, 5 },  // [7]  eight
  {  8,  9, 4 },  // [8]  nine
  { 10, 12, 3 },  // [9]  ten
  {  8,  4, 6 },  // [10] eleven
  {  9, 10, 6 },  // [11] twelve
};

// ── Frame operations ──────────────────────────────────────────────────────────
void clearFrame() {
  memset(gridLit, 0, sizeof(gridLit));
}

void addWordToFrame(const byte theword[3]) {
  byte row = pgm_read_byte(&theword[0]);
  byte col = pgm_read_byte(&theword[1]);
  byte len = pgm_read_byte(&theword[2]);
  for (byte i = 0; i < len; i++) {
    if (row < GRID_ROWS && (col + i) < GRID_COLS) {
      gridLit[row][col + i] = true;
    }
  }
}

// ── Render gridLit to sprite ──────────────────────────────────────────────────
// Uses FreeSansBold9pt7b for each letter cell, lit vs dim.
// Font available via TFT_eSPI / gfxfont.h when LOAD_GFXFF=1 — no explicit include needed.

void renderGrid() {
  gridSprite.fillSprite(colourBg);
  gridSprite.setFreeFont(&FreeSansBold9pt7b);
  gridSprite.setTextDatum(MC_DATUM);

  for (int row = 0; row < GRID_ROWS; row++) {
    for (int col = 0; col < GRID_COLS; col++) {
      char letter = GRID[row][col];
      if (letter == '\0') break;

      int cx = col * CELL_W + CELL_W / 2;
      int cy = row * CELL_H + CELL_H / 2;

      gridSprite.setTextColor(gridLit[row][col] ? colourLit : colourDim);
      char buf[2] = { letter, '\0' };
      gridSprite.drawString(buf, cx, cy);
    }
  }
}

// ── showTimeWords — ported from Brett Oliver time.cpp ─────────────────────────
// Exact logic preserved: individual minutes (not 5-min rounding),
// MINUTE/MINUTES suffix, time-of-day (morning/afternoon/evening/night).
void showTimeWords(uint8_t h, uint8_t m) {
  byte h2 = h;

  // "THE TIME IS" always lit
  addWordToFrame(w_the);
  addWordToFrame(w_time);
  addWordToFrame(w_is);

  if (m == 0) {
    if (h == 0) {
      // Midnight: TWELVE O'CLOCK AT NIGHT
      addWordToFrame(w_hours[11]);
      addWordToFrame(w_oclock);
      addWordToFrame(w_at);
      addWordToFrame(w_night);
    } else if (h == 12) {
      // Noon: TWELVE O'CLOCK IN THE AFTERNOON
      addWordToFrame(w_hours[11]);
      addWordToFrame(w_oclock);
      addWordToFrame(w_in);
      addWordToFrame(w_the_tod);
      addWordToFrame(w_afternoon);
    } else {
      // Regular o'clock — hour displayed in block below
      addWordToFrame(w_oclock);
    }

  } else {
    // Show minute word(s)
    if (m <= 20) {
      addWordToFrame(w_minutes[m - 1]);
    } else if (m < 30) {
      addWordToFrame(w_minutes[19]);        // TWENTY
      addWordToFrame(w_minutes[m - 21]);    // units digit
    } else if (m == 30) {
      addWordToFrame(w_half);
    } else if (m < 40) {
      addWordToFrame(w_minutes[19]);        // TWENTY
      addWordToFrame(w_minutes[60 - m - 21]);
    } else {
      addWordToFrame(w_minutes[60 - m - 1]);
    }

    if (m <= 30) {
      addWordToFrame(w_past);
    } else {
      addWordToFrame(w_to);
      ++h2;
    }
  }

  // Hour word + suffix — skip for midnight/noon (handled above)
  if (!(m == 0 && (h == 0 || h == 12))) {
    if (h2 == 0) {
      addWordToFrame(w_hours[11]);  // twelve
    } else if (h2 <= 12) {
      addWordToFrame(w_hours[h2 - 1]);
    } else {
      addWordToFrame(w_hours[h2 - 13]);
    }

    // MINUTE or MINUTES suffix
    if ((m == 1) || (m == 59)) {
      addWordToFrame(w_minute);
    }
    // Plural MINUTES for non-round, non-single minutes
    if ((m > 1  && m < 5)  || (m > 5  && m < 10) ||
        (m > 10 && m < 15) || (m > 15 && m < 20) ||
        (m > 20 && m < 25) || (m > 25 && m < 30) ||
        (m > 30 && m < 35) || (m > 35 && m < 40) ||
        (m > 40 && m < 45) || (m > 45 && m < 50) ||
        (m > 50 && m < 55) || (m > 55 && m < 59)) {
      addWordToFrame(w_minute);
      addWordToFrame(w_s);
    }

    // Time of day
    if (h < 1) {
      addWordToFrame(w_at);
      addWordToFrame(w_night);
    } else if (h < 12) {
      addWordToFrame(w_in);
      addWordToFrame(w_the_tod);
      addWordToFrame(w_morning);
    } else if (h < 18) {
      addWordToFrame(w_in);
      addWordToFrame(w_the_tod);
      addWordToFrame(w_afternoon);
    } else if (h < 21) {
      addWordToFrame(w_in);
      addWordToFrame(w_the_tod);
      addWordToFrame(w_evening);
    } else {
      addWordToFrame(w_at);
      addWordToFrame(w_night);
    }

    // "A QUARTER" article — lights the 'A' in HALF (row 0 col 13)
    if (m == 15 || m == 45) {
      addWordToFrame(w_a);
    }
  }

  renderGrid();
}

// ── Touch ─────────────────────────────────────────────────────────────────────
static SPIClass            touchSPI(VSPI);
static XPT2046_Touchscreen ts(TOUCH_CS, 255);  // 255 = no IRQ

void initTouch() {
  touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(touchSPI);
  ts.setRotation(0);  // match portrait display rotation
  DBG_INFO("Touch initialised (VSPI CLK=%d MISO=%d MOSI=%d CS=%d)",
           TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
}

static uint32_t touchDownMs  = 0;
static bool     touchWasDown = false;
static uint8_t  brightnessStep = 0;

void handleTouch() {
  if (!ts.touched()) {
    touchWasDown = false;
    return;
  }
  TS_Point p = ts.getPoint();
  if (p.z < TOUCH_PRESSURE_MIN) return;

  uint32_t now = millis();
  if (!touchWasDown) {
    touchWasDown = true;
    touchDownMs  = now;
  } else if ((now - touchDownMs) >= TOUCH_LONG_PRESS_MS) {
    touchWasDown = false;
    const uint8_t levels[4] = {60, 120, 180, 255};
    brightnessStep = (brightnessStep + 1) % BRIGHTNESS_STEPS;
    setBrightness(levels[brightnessStep]);
    DBG_INFO("Long press: brightness step=%d pwm=%d",
             brightnessStep, levels[brightnessStep]);
    delay(TOUCH_DEBOUNCE_MS);
  }
}

// ── Main tick ─────────────────────────────────────────────────────────────────
extern Timezone myTZ;

void wordclockTick() {
  static uint8_t  lastMin  = 255;
  static uint8_t  lastSec  = 255;
  static uint32_t lastLDR  = 0;

  handleTouch();

  uint32_t now = millis();
  if (now - lastLDR >= LDR_UPDATE_MS) {
    lastLDR = now;
    updateBrightness();
  }

  uint8_t h = (uint8_t)myTZ.hour();
  uint8_t m = (uint8_t)myTZ.minute();
  uint8_t s = (uint8_t)myTZ.second();

  // Update status strip every second
  if (s != lastSec) {
    lastSec = s;
    drawStatusStrip(h, m, s,
                    (uint8_t)(myTZ.weekday() - 1),  // ezTime: 1=Sun → 0-based
                    (uint8_t)myTZ.day(),
                    (uint8_t)myTZ.month(),
                    (uint16_t)myTZ.year());
  }

  // Redraw word grid only on minute change
  if (m == lastMin) return;
  lastMin = m;

  DBG_INFO("[WordClock] %02d:%02d", h, m);

  clearFrame();
  showTimeWords(h, m);
  pushGrid();
}
