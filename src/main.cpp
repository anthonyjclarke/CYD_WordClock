#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <ezTime.h>
#include "config.h"
#include "debug.h"
#include "display.h"
#include "wordclock.h"

TFT_eSPI tft;
Timezone myTZ;

// ── Display init ──────────────────────────────────────────────────────────────
static void initDisplay() {
  tft.init();
  tft.setRotation(SCREEN_ROTATION);

  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 0);  // start dark

  initColours();

  ledcWrite(0, BRIGHTNESS_DEFAULT);
  currentBrightness = BRIGHTNESS_DEFAULT;

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

  if (!wm.autoConnect(WIFI_AP_NAME)) {
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

  if (!myTZ.setLocation(F(NTP_TIMEZONE))) {
    DBG_WARN("Time: Olson lookup failed — applying POSIX fallback: %s",
             NTP_POSIX_FALLBACK);
    myTZ.setPosix(NTP_POSIX_FALLBACK);
  }

  DBG_INFO("Time synced: %s  tz=%s offset=%s",
           myTZ.dateTime("H:i:s d-M-Y").c_str(),
           myTZ.dateTime("T").c_str(),
           myTZ.dateTime("O").c_str());
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  DBG_INFO("=== CYD WordClock v" FW_VERSION " starting ===");

  initDisplay();
  initTouch();
  initWiFi();
  initTime();

  DBG_INFO("Free heap: %d bytes", ESP.getFreeHeap());

  // Force first draw immediately
  clearFrame();
  showTimeWords((uint8_t)myTZ.hour(), (uint8_t)myTZ.minute());
  pushGrid();
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
  events();           // ezTime NTP housekeeping
  wordclockTick();
}
