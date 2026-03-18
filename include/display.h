#pragma once
// display.h — sprite management, backlight, status strip

#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern TFT_eSprite gridSprite;  // 240×266 word grid, 8-bit depth

// Resolved colour constants — set in initColours()
extern uint16_t colourLit;
extern uint16_t colourDim;
extern uint16_t colourBg;

// Initialise colours and allocate sprite. Call after tft.init().
void initColours();

// Push gridSprite to screen at (0,0)
void pushGrid();

// Clear gridSprite to background colour (does not push)
void clsGrid();

// Set backlight PWM 0–255
void setBrightness(uint8_t pwm);

extern uint8_t currentBrightness;

// Sample LDR, apply rolling average, update backlight. No-op if LDR_ENABLED=0.
void updateBrightness();

// Fade backlight down, clear sprite, restore brightness (for transitions)
void fade_down();

// Draw the status strip below the grid: time + date.
// h=24h hour, m=minute, s=second, dow=day-of-week(0=Sun), day, month, year
void drawStatusStrip(uint8_t h, uint8_t m, uint8_t s,
                     uint8_t dow, uint8_t day, uint8_t month, uint16_t year);
