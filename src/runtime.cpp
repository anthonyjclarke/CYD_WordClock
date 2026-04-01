#include "runtime.h"
#include "settings.h"
#include "display.h"
#include "wordclock.h"
#include "config.h"
#include "debug.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <ezTime.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern Timezone myTZ;

namespace {

enum PendingAction : uint8_t {
  ACTION_NONE = 0,
  ACTION_WIFI_RESET,
  ACTION_SETTINGS_RESET,
  ACTION_FACTORY_RESET
};

PendingAction pendingAction = ACTION_NONE;

void applyTimezoneSettings() {
  if (!myTZ.setLocation(settings().timezone)) {
    DBG_WARN("Time: Olson lookup failed for %s — applying POSIX fallback: %s",
             settings().timezone, settings().posixFallback);
    myTZ.setPosix(settings().posixFallback);
  }
}

}  // namespace

void applyRuntimeSettings(bool redrawNow) {
  tft.setRotation(currentScreenRotation());
  updateTouchRotation(currentScreenRotation());
  setBrightness(settings().brightnessDefault);
  applyTimezoneSettings();

  if (redrawNow) {
    invalidateStatusStrip();
    redrawClockNow();
  }
}

void requestWifiReset() {
  pendingAction = ACTION_WIFI_RESET;
}

void requestSettingsReset() {
  pendingAction = ACTION_SETTINGS_RESET;
}

void requestFactoryReset() {
  pendingAction = ACTION_FACTORY_RESET;
}

void processPendingSystemActions() {
  if (pendingAction == ACTION_NONE) return;

  PendingAction action = pendingAction;
  pendingAction = ACTION_NONE;

  if (action == ACTION_SETTINGS_RESET) {
    resetSettingsToDefaults();
    applyRuntimeSettings(true);
    DBG_INFO("Settings reset to defaults");
    return;
  }

  WiFiManager wm;
  if (action == ACTION_FACTORY_RESET) {
    clearStoredSettings();
    DBG_INFO("Factory reset: cleared stored settings");
  }

  wm.resetSettings();
  WiFi.disconnect(true, true);
  delay(250);
  ESP.restart();
}
