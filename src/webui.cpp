#include "webui.h"
#include "settings.h"
#include "runtime.h"
#include "wordclock.h"
#include "display.h"
#include "config.h"

#include <WebServer.h>
#include <WiFi.h>
#include <esp_system.h>
#include <ezTime.h>

extern Timezone myTZ;

namespace {

WebServer server(80);

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>CYD Word Clock</title>
  <style>
    :root {
      --bg: #080808;
      --panel: #121212;
      --text: #f2f2df;
      --muted: #777;
      --dim: rgb(35,35,35);
      --lit: rgb(255,255,210);
      --line: #262626;
      --accent: #d5c47b;
      --status: rgb(120,120,120);
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "SF Mono", "Roboto Mono", "Menlo", monospace;
      background:
        radial-gradient(circle at top, rgba(213,196,123,.08), transparent 35%),
        linear-gradient(180deg, #111 0%, #050505 100%);
      color: var(--text);
      min-height: 100vh;
      padding: 24px;
    }
    .wrap { max-width: 960px; margin: 0 auto; }
    .tabs { display: flex; gap: 10px; margin-bottom: 18px; }
    .tab-btn {
      border: 1px solid var(--line);
      background: rgba(255,255,255,.04);
      color: var(--text);
      padding: 10px 14px;
      cursor: pointer;
    }
    .tab-btn.active { border-color: var(--accent); color: var(--accent); }
    .tab { display: none; }
    .tab.active { display: block; }
    .stack { display: grid; gap: 20px; }
    .clock-panel {
      display: grid;
      gap: 20px;
      justify-items: center;
    }
    .clock-toolbar {
      width: 100%;
      display: flex;
      justify-content: center;
    }
    .toggle {
      display: inline-flex;
      align-items: center;
      gap: 10px;
      margin: 0;
      padding: 10px 14px;
      border: 1px solid var(--line);
      background: rgba(255,255,255,.03);
      font-size: 13px;
    }
    .toggle input {
      width: 16px;
      height: 16px;
      margin: 0;
      accent-color: var(--accent);
    }
    .clock-stage {
      --clock-scale: 1;
      width: 100%;
      min-height: calc(320px * var(--clock-scale) + 24px);
      display: flex;
      justify-content: center;
      align-items: flex-start;
      overflow: auto;
      padding: 12px;
    }
    .clock-stage.double {
      --clock-scale: 2;
    }
    .clock-shell {
      width: 240px;
      height: 320px;
      flex: 0 0 auto;
      background: var(--bg);
      border: 1px solid var(--line);
      box-shadow: 0 16px 40px rgba(0,0,0,.45);
      overflow: hidden;
      transform: scale(var(--clock-scale));
      transform-origin: top center;
      transition: transform 160ms ease;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(16, 1fr);
      grid-template-rows: repeat(14, 19px);
      height: 266px;
      align-items: center;
      justify-items: center;
      padding-top: 0;
    }
    .cell {
      width: 15px;
      height: 19px;
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--dim);
      font-size: 13px;
      font-weight: 700;
      line-height: 1;
    }
    .cell.lit { color: var(--lit); }
    .status-strip {
      margin-top: 2px;
      height: 52px;
      display: grid;
      place-items: start center;
      color: var(--status);
      padding-top: 4px;
      text-align: center;
      font-family: Arial, sans-serif;
      font-weight: 700;
    }
    .status-time { font-size: 18px; line-height: 1.1; }
    .status-date { font-size: 18px; line-height: 1.1; margin-top: 6px; }
    .panel {
      background: rgba(255,255,255,.03);
      border: 1px solid var(--line);
      padding: 18px;
    }
    .grid-two { display: grid; gap: 18px; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); }
    .section-title { margin: 0 0 12px; font-size: 14px; letter-spacing: .08em; color: var(--accent); text-transform: uppercase; }
    label { display: grid; gap: 6px; margin-bottom: 12px; font-size: 13px; }
    input, select, button {
      width: 100%;
      padding: 10px;
      background: #0e0e0e;
      color: var(--text);
      border: 1px solid var(--line);
    }
    button { cursor: pointer; }
    .actions { display: grid; gap: 10px; }
    .meta { color: var(--muted); font-size: 13px; }
    .footer {
      margin-top: 18px;
      display: flex;
      gap: 16px;
      flex-wrap: wrap;
      font-size: 13px;
    }
    .footer a { color: var(--accent); text-decoration: none; }
    pre {
      margin: 0;
      white-space: pre-wrap;
      font-size: 12px;
      color: #ddd;
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="tabs">
      <button class="tab-btn active" data-tab="clock">Clock</button>
      <button class="tab-btn" data-tab="config">Config</button>
    </div>

    <section class="tab active" id="clock">
      <div class="stack">
        <div class="panel">
          <div class="clock-panel">
            <div class="clock-toolbar">
              <label class="toggle">
                <input type="checkbox" id="clock-scale-toggle">
                <span>Double Size (2x)</span>
              </label>
            </div>
            <div class="clock-stage" id="clock-stage">
              <div class="clock-shell">
                <div class="grid" id="clock-grid"></div>
                <div class="status-strip">
                  <div class="status-time" id="status-time">--:--:--</div>
                  <div class="status-date" id="status-date">---</div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
      <div class="footer">
        <a href="https://github.com/anthonyjclarke/CYD_WordClock" target="_blank" rel="noreferrer">GitHub</a>
        <a href="https://bsky.app/profile/anthonyjclarke.bsky.social" target="_blank" rel="noreferrer">Bluesky</a>
      </div>
    </section>

    <section class="tab" id="config">
      <div class="grid-two">
        <form class="panel" id="config-form">
          <h2 class="section-title">Display</h2>
          <label>Display Flip
            <select name="display_flip">
              <option value="0">USB Bottom</option>
              <option value="1">USB Top</option>
            </select>
          </label>
          <label>Default Brightness
            <input type="number" min="0" max="255" name="brightness_default">
          </label>
          <label>Brightness Min
            <input type="number" min="0" max="255" name="brightness_min">
          </label>
          <label>Brightness Max
            <input type="number" min="0" max="255" name="brightness_max">
          </label>
          <label>Brightness Steps
            <input type="number" min="1" max="8" name="brightness_steps">
          </label>

          <h2 class="section-title">Animation</h2>
          <label>Animation Type
            <select name="anim_type">
              <option value="0">Instant</option>
              <option value="1">Fade</option>
            </select>
          </label>
          <label>Fade Steps
            <input type="number" min="1" max="40" name="anim_fade_steps">
          </label>
          <label>Fade Frame Delay (ms)
            <input type="number" min="1" max="1000" name="anim_fade_ms">
          </label>

          <h2 class="section-title">Brightness / LDR</h2>
          <label>LDR Auto Brightness
            <select name="ldr_enabled">
              <option value="0">Disabled</option>
              <option value="1">Enabled</option>
            </select>
          </label>
          <label>LDR Dark
            <input type="number" min="1" max="4095" name="ldr_dark">
          </label>
          <label>LDR Bright
            <input type="number" min="2" max="4095" name="ldr_bright">
          </label>

          <h2 class="section-title">Timezone</h2>
          <label>Timezone
            <input type="text" name="timezone">
          </label>
          <label>POSIX Fallback
            <input type="text" name="posix_fallback">
          </label>

          <div class="actions">
            <button type="submit">Save Live Settings</button>
          </div>
          <p class="meta" id="save-status"></p>
        </form>

        <div class="panel">
          <h2 class="section-title">Maintenance</h2>
          <div class="actions">
            <button type="button" id="reset-settings">Reset Settings To Defaults</button>
            <button type="button" id="reset-wifi">Reset WiFi And Reboot</button>
            <button type="button" id="factory-reset">Factory Reset And Reboot</button>
          </div>
          <p class="meta">Settings save to NVS and apply immediately where supported.</p>

          <h2 class="section-title">System Information</h2>
          <pre id="system-info-config">Loading...</pre>
        </div>
      </div>
      <div class="footer">
        <a href="https://github.com/anthonyjclarke/CYD_WordClock" target="_blank" rel="noreferrer">GitHub</a>
        <a href="https://bsky.app/profile/anthonyjclarke.bsky.social" target="_blank" rel="noreferrer">Bluesky</a>
      </div>
    </section>
  </div>

  <script>
    const gridEl = document.getElementById('clock-grid');
    const clockStage = document.getElementById('clock-stage');
    const clockScaleToggle = document.getElementById('clock-scale-toggle');
    const tabs = document.querySelectorAll('.tab-btn');
    const tabPanels = document.querySelectorAll('.tab');
    for (let row = 0; row < 14; row++) {
      for (let col = 0; col < 16; col++) {
        const cell = document.createElement('div');
        cell.className = 'cell';
        cell.id = `cell-${row}-${col}`;
        gridEl.appendChild(cell);
      }
    }

    tabs.forEach(btn => btn.addEventListener('click', () => {
      tabs.forEach(x => x.classList.remove('active'));
      tabPanels.forEach(x => x.classList.remove('active'));
      btn.classList.add('active');
      document.getElementById(btn.dataset.tab).classList.add('active');
    }));

    clockScaleToggle.addEventListener('change', () => {
      clockStage.classList.toggle('double', clockScaleToggle.checked);
    });

    function setSystemInfo(state) {
      const lines = [
        `Firmware: ${state.system.firmware}`,
        `Uptime: ${state.system.uptime_s}s`,
        `Time: ${state.time} ${state.date}`,
        `Timezone: ${state.settings.timezone}`,
        `WiFi: ${state.system.wifi_status}`,
        `SSID: ${state.system.ssid}`,
        `IP: ${state.system.ip}`,
        `RSSI: ${state.system.rssi}`,
        `Heap: ${state.system.free_heap}`,
        `Max Alloc Heap: ${state.system.max_alloc_heap}`,
        `Flash Size: ${state.system.flash_size}`,
        `Sketch Size: ${state.system.sketch_size}`,
        `Free Sketch Space: ${state.system.free_sketch_space}`,
        `Display Flip: ${state.settings.display_flip}`,
        `Brightness Default: ${state.settings.brightness_default}`,
        `Animation: ${state.settings.anim_type}`
      ].join('\n');
      document.getElementById('system-info-config').textContent = lines;
    }

    async function fetchState() {
      const response = await fetch('/api/state');
      const state = await response.json();
      document.getElementById('status-time').textContent = state.time;
      document.getElementById('status-date').textContent = state.date;
      state.grid.rows.forEach((rowText, rowIdx) => {
        const litRow = state.grid.lit[rowIdx];
        rowText.split('').forEach((ch, colIdx) => {
          const cell = document.getElementById(`cell-${rowIdx}-${colIdx}`);
          cell.textContent = ch;
          cell.classList.toggle('lit', litRow[colIdx] === '1');
        });
      });
      setSystemInfo(state);
    }

    async function fetchConfig() {
      const response = await fetch('/api/config');
      const cfg = await response.json();
      for (const [key, value] of Object.entries(cfg)) {
        const field = document.querySelector(`[name="${key}"]`);
        if (field) field.value = value;
      }
    }

    async function postForm(url, body) {
      return fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams(body)
      });
    }

    document.getElementById('config-form').addEventListener('submit', async (event) => {
      event.preventDefault();
      const form = new FormData(event.target);
      const body = Object.fromEntries(form.entries());
      const response = await postForm('/api/config', body);
      document.getElementById('save-status').textContent = response.ok ? 'Settings saved and applied.' : 'Save failed.';
      await fetchConfig();
      await fetchState();
    });

    document.getElementById('reset-settings').addEventListener('click', async () => {
      await postForm('/api/reset/settings', {});
      document.getElementById('save-status').textContent = 'Settings reset to defaults.';
      await fetchConfig();
      await fetchState();
    });

    document.getElementById('reset-wifi').addEventListener('click', async () => {
      await postForm('/api/reset/wifi', {});
      document.getElementById('save-status').textContent = 'WiFi reset requested. Device will reboot.';
    });

    document.getElementById('factory-reset').addEventListener('click', async () => {
      await postForm('/api/reset/all', {});
      document.getElementById('save-status').textContent = 'Factory reset requested. Device will reboot.';
    });

    fetchConfig();
    fetchState();
    setInterval(fetchState, 1000);
    setInterval(fetchConfig, 5000);
  </script>
</body>
</html>
)HTML";

String jsonEscape(const String& value) {
  String out;
  out.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];
    if (c == '\\' || c == '"') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else {
      out += c;
    }
  }
  return out;
}

String dayDateString() {
  static const char *dayAbbr[7]  = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
  static const char *monAbbr[12] = {"JAN","FEB","MAR","APR","MAY","JUN",
                                    "JUL","AUG","SEP","OCT","NOV","DEC"};
  uint8_t dow = (uint8_t)(myTZ.weekday() - 1);
  uint8_t mon = (uint8_t)myTZ.month();
  if (dow > 6 || mon < 1 || mon > 12) return "---";

  char buf[20];
  snprintf(buf, sizeof(buf), "%s %02d %s %04d",
           dayAbbr[dow], (uint8_t)myTZ.day(), monAbbr[mon - 1], (uint16_t)myTZ.year());
  return String(buf);
}

String buildStateJson() {
  String json;
  json.reserve(2500);
  json += "{";
  json += "\"time\":\"" + jsonEscape(myTZ.dateTime("H:i:s")) + "\",";
  json += "\"date\":\"" + jsonEscape(dayDateString()) + "\",";
  json += "\"grid\":{";
  json += "\"rows\":[";
  for (uint8_t row = 0; row < GRID_ROWS; ++row) {
    if (row) json += ",";
    json += "\"" + jsonEscape(String(getGridRowText(row))) + "\"";
  }
  json += "],\"lit\":[";
  for (uint8_t row = 0; row < GRID_ROWS; ++row) {
    if (row) json += ",";
    String litRow;
    litRow.reserve(GRID_COLS);
    for (uint8_t col = 0; col < GRID_COLS; ++col) litRow += gridLit[row][col] ? '1' : '0';
    json += "\"" + litRow + "\"";
  }
  json += "]},";
  json += "\"settings\":{";
  json += "\"display_flip\":" + String(settings().displayFlip ? 1 : 0) + ",";
  json += "\"brightness_default\":" + String(settings().brightnessDefault) + ",";
  json += "\"anim_type\":" + String(settings().animType) + ",";
  json += "\"timezone\":\"" + jsonEscape(String(settings().timezone)) + "\"";
  json += "},";
  json += "\"system\":{";
  json += "\"firmware\":\"" FW_VERSION "\",";
  json += "\"uptime_s\":" + String(millis() / 1000UL) + ",";
  json += "\"wifi_status\":\"" + String(WiFi.status() == WL_CONNECTED ? "connected" : "offline") + "\",";
  json += "\"ssid\":\"" + jsonEscape(WiFi.SSID()) + "\",";
  json += "\"ip\":\"" + jsonEscape(WiFi.isConnected() ? WiFi.localIP().toString() : String("0.0.0.0")) + "\",";
  json += "\"rssi\":" + String(WiFi.isConnected() ? WiFi.RSSI() : 0) + ",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"max_alloc_heap\":" + String(ESP.getMaxAllocHeap()) + ",";
  json += "\"flash_size\":" + String(ESP.getFlashChipSize()) + ",";
  json += "\"sketch_size\":" + String(ESP.getSketchSize()) + ",";
  json += "\"free_sketch_space\":" + String(ESP.getFreeSketchSpace());
  json += "}}";
  return json;
}

String buildConfigJson() {
  String json;
  json.reserve(512);
  json += "{";
  json += "\"display_flip\":" + String(settings().displayFlip ? 1 : 0) + ",";
  json += "\"brightness_default\":" + String(settings().brightnessDefault) + ",";
  json += "\"brightness_min\":" + String(settings().brightnessMin) + ",";
  json += "\"brightness_max\":" + String(settings().brightnessMax) + ",";
  json += "\"brightness_steps\":" + String(settings().brightnessSteps) + ",";
  json += "\"ldr_enabled\":" + String(settings().ldrEnabled ? 1 : 0) + ",";
  json += "\"ldr_dark\":" + String(settings().ldrDark) + ",";
  json += "\"ldr_bright\":" + String(settings().ldrBright) + ",";
  json += "\"anim_type\":" + String(settings().animType) + ",";
  json += "\"anim_fade_steps\":" + String(settings().animFadeSteps) + ",";
  json += "\"anim_fade_ms\":" + String(settings().animFadeMs) + ",";
  json += "\"timezone\":\"" + jsonEscape(String(settings().timezone)) + "\",";
  json += "\"posix_fallback\":\"" + jsonEscape(String(settings().posixFallback)) + "\"";
  json += "}";
  return json;
}

void sendJson(const String& json) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleState() {
  sendJson(buildStateJson());
}

void handleConfigGet() {
  sendJson(buildConfigJson());
}

void handleConfigPost() {
  RuntimeSettings next = settings();
  next.displayFlip = server.arg("display_flip").toInt() != 0;
  next.brightnessDefault = (uint8_t)server.arg("brightness_default").toInt();
  next.brightnessMin = (uint8_t)server.arg("brightness_min").toInt();
  next.brightnessMax = (uint8_t)server.arg("brightness_max").toInt();
  next.brightnessSteps = (uint8_t)server.arg("brightness_steps").toInt();
  next.ldrEnabled = server.arg("ldr_enabled").toInt() != 0;
  next.ldrDark = (uint16_t)server.arg("ldr_dark").toInt();
  next.ldrBright = (uint16_t)server.arg("ldr_bright").toInt();
  next.animType = (uint8_t)server.arg("anim_type").toInt();
  next.animFadeSteps = (uint8_t)server.arg("anim_fade_steps").toInt();
  next.animFadeMs = (uint16_t)server.arg("anim_fade_ms").toInt();
  strlcpy(next.timezone, server.arg("timezone").c_str(), sizeof(next.timezone));
  strlcpy(next.posixFallback, server.arg("posix_fallback").c_str(), sizeof(next.posixFallback));

  saveSettings(next);
  applyRuntimeSettings(true);
  sendJson("{\"ok\":true}");
}

void handleResetSettings() {
  requestSettingsReset();
  sendJson("{\"ok\":true}");
}

void handleResetWifi() {
  requestWifiReset();
  sendJson("{\"ok\":true,\"reboot\":true}");
}

void handleResetAll() {
  requestFactoryReset();
  sendJson("{\"ok\":true,\"reboot\":true}");
}

}  // namespace

void initWebUi() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/config", HTTP_GET, handleConfigGet);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/reset/settings", HTTP_POST, handleResetSettings);
  server.on("/api/reset/wifi", HTTP_POST, handleResetWifi);
  server.on("/api/reset/all", HTTP_POST, handleResetAll);
  server.begin();
}

void webUiTick() {
  server.handleClient();
}
