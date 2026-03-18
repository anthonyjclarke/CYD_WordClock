#include "display.h"
#include "config.h"
#include "debug.h"
#include <Arduino.h>
// FreeSansBold9pt7b is pulled in by TFT_eSPI via gfxfont.h when LOAD_GFXFF=1
// No explicit include needed — use &FreeSansBold9pt7b directly.

TFT_eSprite gridSprite(&tft);

uint8_t  currentBrightness = BRIGHTNESS_DEFAULT;
uint16_t colourLit = 0;
uint16_t colourDim = 0;
uint16_t colourBg  = 0;

void initColours() {
  colourLit = tft.color565(COLOUR_LIT_R,  COLOUR_LIT_G,  COLOUR_LIT_B);
  colourDim = tft.color565(COLOUR_DIM_R,  COLOUR_DIM_G,  COLOUR_DIM_B);
  colourBg  = tft.color565(COLOUR_BG_R,   COLOUR_BG_G,   COLOUR_BG_B);

  // 8-bit depth: 240×266×1 = 63,840 bytes — fits comfortably without PSRAM
  gridSprite.setColorDepth(8);
  void* buf = gridSprite.createSprite(240, GRID_HEIGHT);
  if (!buf) {
    DBG_ERROR("gridSprite alloc FAILED — heap %d bytes, max block %d bytes",
              ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  }
  gridSprite.fillSprite(colourBg);

  tft.fillScreen(colourBg);

  DBG_INFO("Display colours initialised, sprite 240x%d depth=8 heap=%d",
           GRID_HEIGHT, ESP.getFreeHeap());
}

void pushGrid() {
  tft.startWrite();
  gridSprite.pushSprite(0, 0);
  tft.endWrite();
}

void clsGrid() {
  gridSprite.fillSprite(colourBg);
}

void setBrightness(uint8_t pwm) {
  currentBrightness = pwm;
  ledcWrite(0, pwm);
}

void fade_down() {
  for (int i = currentBrightness; i >= 0; i -= 8) {
    ledcWrite(0, (uint8_t)i);
    delay(20);
  }
  ledcWrite(0, 0);
  clsGrid();
  pushGrid();
  ledcWrite(0, currentBrightness);
}

void updateBrightness() {
#if LDR_ENABLED
  static uint16_t buf[LDR_SAMPLES] = {};
  static byte     idx  = 0;
  static bool     full = false;

  buf[idx] = (uint16_t)analogRead(LDR_PIN);
  idx = (idx + 1) % LDR_SAMPLES;
  if (idx == 0) full = true;

  byte     count = full ? LDR_SAMPLES : idx;
  if (count == 0) return;

  uint32_t sum = 0;
  for (byte i = 0; i < count; i++) sum += buf[i];
  uint16_t avg = (uint16_t)(sum / count);

  uint8_t b = (uint8_t)constrain(
    map((long)avg, (long)LDR_DARK, (long)LDR_BRIGHT,
        (long)BRIGHTNESS_MIN, (long)BRIGHTNESS_MAX),
    (long)BRIGHTNESS_MIN, (long)BRIGHTNESS_MAX
  );
  setBrightness(b);
  DBG_VERBOSE("LDR avg=%u → brightness=%u", (unsigned)avg, (unsigned)b);
#endif
}

void drawStatusStrip(uint8_t h, uint8_t m, uint8_t s,
                     uint8_t dow, uint8_t day, uint8_t month, uint16_t year) {
  static const char *dayAbbr[7]  = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
  static const char *monAbbr[12] = {"JAN","FEB","MAR","APR","MAY","JUN",
                                     "JUL","AUG","SEP","OCT","NOV","DEC"};

  uint16_t statusColour = tft.color565(COLOUR_STATUS_R, COLOUR_STATUS_G, COLOUR_STATUS_B);

  // Fill status strip background
  tft.fillRect(0, STATUS_Y, 240, STATUS_HEIGHT, colourBg);

  // Time line: HH:MM:SS centred
  char timeBuf[9];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", h, m, s);

  // Date line: DOW DD MON YYYY centred
  char dateBuf[16];
  if (dow <= 6 && month >= 1 && month <= 12) {
    snprintf(dateBuf, sizeof(dateBuf), "%s %02d %s %04d",
             dayAbbr[dow], day, monAbbr[month - 1], year);
  } else {
    snprintf(dateBuf, sizeof(dateBuf), "--:--:--");
  }

  // FreeSansBold9pt7b: cap height ~9px, baseline-to-top ~12px
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(statusColour, colourBg);
  tft.setTextDatum(TC_DATUM);  // top-centre

  // Time on first line (y = top of text)
  tft.drawString(timeBuf, 120, STATUS_Y + 4);

  // Date on second line
  tft.drawString(dateBuf, 120, STATUS_Y + 24);
}
