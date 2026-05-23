// ═══════════════════════════════════════════════════════════
//  00_config.ino - Global config, structs, shared state
//  Definitions live here; declarations are in config.h
// ═══════════════════════════════════════════════════════════

#include "config.h"

// ── Object definitions ────────────────────────────────────
Adafruit_SSD1306  oled(OLED_W, OLED_H, &Wire, -1);
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231        rtc;
Preferences       prefs;
WiFiServer        transferServer(TRANSFER_PORT);

// ── Config instance ───────────────────────────────────────
Config cfg;

// ── App state ─────────────────────────────────────────────
AppMode currentMode = MODE_CLOCK;
bool sdReady    = false;
bool rtcReady   = false;
bool wifiReady  = false;
bool appRunning = false;

// ── Glyph table ───────────────────────────────────────────
GlyphEntry glyphTable[MAX_CHARS];
uint16_t   glyphCount = 0;

// ── Settings save/load ────────────────────────────────────
void saveSettings() {
  prefs.begin("mpc", false);
  prefs.putString("wssid",   cfg.wifiSSID);
  prefs.putString("wpass",   cfg.wifiPass);
  prefs.putString("aiip",    cfg.aiIP);
  prefs.putInt   ("aiport",  cfg.aiPort);
  prefs.putString("aimodel", cfg.aiModel);
  prefs.putString("aisys",   cfg.aiSysPrompt);
  prefs.putUChar ("aidisp",  cfg.aiDispMode);
  prefs.putUChar ("qtrans",  cfg.quranTrans);
  prefs.putBool  ("sound",   cfg.soundEnabled);
  prefs.putUChar ("bright",  cfg.brightness);
  prefs.putUChar ("theme",   cfg.theme);
  prefs.putString("appsp",   cfg.appsPath);
  prefs.putInt   ("wto",     cfg.wifiTimeout);
  prefs.end();
}

void loadSettings() {
  prefs.begin("mpc", true);
  cfg.wifiSSID     = prefs.getString("wssid",   "");
  cfg.wifiPass     = prefs.getString("wpass",   "");
  cfg.aiIP         = prefs.getString("aiip",    "");
  cfg.aiPort       = prefs.getInt   ("aiport",  8081);
  cfg.aiModel      = prefs.getString("aimodel", "");
  cfg.aiSysPrompt  = prefs.getString("aisys",   "You are a helpful assistant.");
  cfg.aiDispMode   = prefs.getUChar ("aidisp",  0);
  cfg.quranTrans   = prefs.getUChar ("qtrans",  0);
  cfg.soundEnabled = prefs.getBool  ("sound",   true);
  cfg.brightness   = prefs.getUChar ("bright",  200);
  cfg.theme        = prefs.getUChar ("theme",   0);
  cfg.appsPath     = prefs.getString("appsp",   "/apps/");
  cfg.wifiTimeout  = prefs.getInt   ("wto",     10);
  prefs.end();
}
