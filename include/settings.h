#pragma once

#include <Arduino.h>

struct RuntimeSettings {
  bool     displayFlip;
  uint8_t  brightnessDefault;
  uint8_t  brightnessMin;
  uint8_t  brightnessMax;
  uint8_t  brightnessSteps;
  bool     ldrEnabled;
  uint16_t ldrDark;
  uint16_t ldrBright;
  uint8_t  animType;
  uint8_t  animFadeSteps;
  uint16_t animFadeMs;
  char     timezone[40];
  char     posixFallback[64];
};

const RuntimeSettings& settings();
void initSettings();
bool saveSettings(const RuntimeSettings& next);
void resetSettingsToDefaults();
void clearStoredSettings();
uint8_t currentScreenRotation();
