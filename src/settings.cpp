#include "settings.h"
#include "config.h"

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>

namespace {

Preferences prefs;
RuntimeSettings g_settings;

RuntimeSettings makeDefaults() {
  RuntimeSettings s{};
  s.displayFlip = DISPLAY_FLIP;
  s.brightnessDefault = BRIGHTNESS_DEFAULT;
  s.brightnessMin = BRIGHTNESS_MIN;
  s.brightnessMax = BRIGHTNESS_MAX;
  s.brightnessSteps = BRIGHTNESS_STEPS;
  s.ldrEnabled = LDR_ENABLED;
  s.ldrDark = LDR_DARK;
  s.ldrBright = LDR_BRIGHT;
  s.animType = ANIM_TYPE;
  s.animFadeSteps = ANIM_FADE_STEPS;
  s.animFadeMs = ANIM_FADE_MS;
  strlcpy(s.timezone, NTP_TIMEZONE, sizeof(s.timezone));
  strlcpy(s.posixFallback, NTP_POSIX_FALLBACK, sizeof(s.posixFallback));
  return s;
}

void clampSettings(RuntimeSettings& s) {
  s.brightnessMin = constrain(s.brightnessMin, (uint8_t)0, (uint8_t)255);
  s.brightnessMax = constrain(s.brightnessMax, s.brightnessMin, (uint8_t)255);
  s.brightnessDefault = constrain(s.brightnessDefault, s.brightnessMin, s.brightnessMax);
  s.brightnessSteps = constrain(s.brightnessSteps, (uint8_t)1, (uint8_t)8);
  s.ldrDark = max<uint16_t>(s.ldrDark, 1);
  s.ldrBright = max<uint16_t>(s.ldrBright, (uint16_t)(s.ldrDark + 1));
  s.animType = (s.animType == ANIM_FADE) ? ANIM_FADE : ANIM_NONE;
  s.animFadeSteps = constrain(s.animFadeSteps, (uint8_t)1, (uint8_t)40);
  s.animFadeMs = max<uint16_t>(1, min<uint16_t>(s.animFadeMs, 1000));
  if (s.timezone[0] == '\0') strlcpy(s.timezone, NTP_TIMEZONE, sizeof(s.timezone));
  if (s.posixFallback[0] == '\0') strlcpy(s.posixFallback, NTP_POSIX_FALLBACK, sizeof(s.posixFallback));
}

void saveToPrefs(const RuntimeSettings& s) {
  prefs.putBool("disp_flip", s.displayFlip);
  prefs.putUChar("bri_def", s.brightnessDefault);
  prefs.putUChar("bri_min", s.brightnessMin);
  prefs.putUChar("bri_max", s.brightnessMax);
  prefs.putUChar("bri_step", s.brightnessSteps);
  prefs.putBool("ldr_en", s.ldrEnabled);
  prefs.putUShort("ldr_dark", s.ldrDark);
  prefs.putUShort("ldr_bright", s.ldrBright);
  prefs.putUChar("anim_type", s.animType);
  prefs.putUChar("fade_steps", s.animFadeSteps);
  prefs.putUShort("fade_ms", s.animFadeMs);
  prefs.putString("tz_name", s.timezone);
  prefs.putString("tz_posix", s.posixFallback);
}

void loadFromPrefs(RuntimeSettings& s) {
  s.displayFlip = prefs.getBool("disp_flip", s.displayFlip);
  s.brightnessDefault = prefs.getUChar("bri_def", s.brightnessDefault);
  s.brightnessMin = prefs.getUChar("bri_min", s.brightnessMin);
  s.brightnessMax = prefs.getUChar("bri_max", s.brightnessMax);
  s.brightnessSteps = prefs.getUChar("bri_step", s.brightnessSteps);
  s.ldrEnabled = prefs.getBool("ldr_en", s.ldrEnabled);
  s.ldrDark = prefs.getUShort("ldr_dark", s.ldrDark);
  s.ldrBright = prefs.getUShort("ldr_bright", s.ldrBright);
  s.animType = prefs.getUChar("anim_type", s.animType);
  s.animFadeSteps = prefs.getUChar("fade_steps", s.animFadeSteps);
  s.animFadeMs = prefs.getUShort("fade_ms", s.animFadeMs);

  String tz = prefs.getString("tz_name", s.timezone);
  String posix = prefs.getString("tz_posix", s.posixFallback);
  strlcpy(s.timezone, tz.c_str(), sizeof(s.timezone));
  strlcpy(s.posixFallback, posix.c_str(), sizeof(s.posixFallback));
}

}  // namespace

const RuntimeSettings& settings() {
  return g_settings;
}

void initSettings() {
  g_settings = makeDefaults();
  prefs.begin("wordclock", false);
  loadFromPrefs(g_settings);
  clampSettings(g_settings);
  saveToPrefs(g_settings);
}

bool saveSettings(const RuntimeSettings& next) {
  g_settings = next;
  clampSettings(g_settings);
  saveToPrefs(g_settings);
  return true;
}

void resetSettingsToDefaults() {
  g_settings = makeDefaults();
  clampSettings(g_settings);
  saveToPrefs(g_settings);
}

void clearStoredSettings() {
  prefs.clear();
}

uint8_t currentScreenRotation() {
  return settings().displayFlip ? 2 : 0;
}
