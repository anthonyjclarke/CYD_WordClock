#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <ezTime.h>
#include <esp_ota_ops.h>
#include "config.h"
#include "debug.h"
#include "display.h"
#include "settings.h"
#include "runtime.h"
#include "webui.h"
#include "wordclock.h"
#include "network/improv_setup.h"

TFT_eSPI tft;
Timezone myTZ;

// ── Display init ──────────────────────────────────────────────────────────────
static void initDisplay() {
  tft.init();
  tft.setRotation(currentScreenRotation());

  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 0);  // start dark

  initColours();

  ledcWrite(0, settings().brightnessDefault);
  currentBrightness = settings().brightnessDefault;

  DBG_INFO("Display initialised %dx%d rotation=%d",
           tft.width(), tft.height(), SCREEN_ROTATION);
}

// ── Status message during boot (draws direct to screen) ───────────────────────
static void showStatus(const char* msg) {
  tft.setFreeFont(NULL);  // revert to built-in for boot messages only
  tft.setTextColor(tft.color565(120, 120, 120), tft.color565(8, 8, 8));
  tft.setTextSize(1);
  tft.drawString(msg, 10, 300, 2);
}

// ── WiFi init ─────────────────────────────────────────────────────────────────
static void initWiFi() {
  showStatus("Connecting WiFi...");
  WiFiManager wm;
  wm.setConfigPortalTimeout(WIFI_TIMEOUT_S);
#if IMPROV_SETUP_ENABLED
  // Non-blocking portal so Improv-Serial (installer "Configure WiFi") is
  // serviced alongside it; setup() still waits here as before.
  wm.setConfigPortalBlocking(false);
#endif

  bool connected = wm.autoConnect(AP_NAME);
#if IMPROV_SETUP_ENABLED
  while (!connected && wm.getConfigPortalActive()) {
    if (wm.process()) { connected = true; break; }
    improvTick();  // restarts once Improv credentials connect
    delay(5);
  }
#endif

  if (!connected) {
    DBG_WARN("WiFi: connect timeout, continuing offline");
    showStatus("WiFi offline     ");
  } else {
    DBG_INFO("WiFi connected: %s", WiFi.localIP().toString().c_str());
    showStatus("WiFi OK          ");
  }
}

// ── Time init ─────────────────────────────────────────────────────────────────
static void initTime() {
  showStatus("Syncing NTP...   ");
  waitForSync(NTP_SYNC_TIMEOUT_S);

  if (timeStatus() == timeNotSet) {
    DBG_WARN("Time: NTP sync failed — clock will show boot time");
  }

  if (!myTZ.setLocation(settings().timezone)) {
    DBG_WARN("Time: Olson lookup failed — applying POSIX fallback: %s",
             settings().posixFallback);
    myTZ.setPosix(settings().posixFallback);
  }

  DBG_INFO("Time synced: %s  tz=%s offset=%s",
           myTZ.dateTime("H:i:s d-M-Y").c_str(),
           myTZ.dateTime("T").c_str(),
           myTZ.dateTime("O").c_str());
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  improvBegin();
  DBG_INFO("=== CYD WordClock v" FIRMWARE_VERSION " starting ===");
  DBG_INFO("Running from %s", esp_ota_get_running_partition()->label);

  initSettings();
  initDisplay();
  initTouch();
  initWiFi();
  initTime();
  initWebUi();
  applyRuntimeSettings(false);

  DBG_INFO("Free heap: %d bytes", ESP.getFreeHeap());

  // Force first draw immediately
  clearFrame();
  showTimeWords((uint8_t)myTZ.hour(), (uint8_t)myTZ.minute());
  pushGrid();
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
  improvTick();       // web installer over USB; must run at least every ~1 s
  events();           // ezTime NTP housekeeping
  webUiTick();
  processPendingSystemActions();
  wordclockTick();
}
