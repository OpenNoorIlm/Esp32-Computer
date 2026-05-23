#pragma once

// ═══════════════════════════════════════════════════════════
//  config.h - Global config, structs, shared state
//  Extracted from 00_config.ino so main.ino can #include it
//  first, fixing the Arduino file-ordering problem.
// ═══════════════════════════════════════════════════════════

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#define LUA_USE_C89
#include <LuaWrapper.h>

// ── Pins ─────────────────────────────────────────────────
#define BUZZER_PIN    4
#define RGB_R         33
#define RGB_G         25
#define RGB_B         32
#define SD_CS         5
#define OLED_W        128
#define OLED_H        64
#define OLED_ADDR     0x3C
#define KB_ADDR       0x12
#define CHAR_W        8
#define CHAR_H        8
#define MAX_CHARS     300
#define TRANSFER_PORT 11211

// ── Objects ───────────────────────────────────────────────
extern Adafruit_SSD1306  oled;
extern LiquidCrystal_I2C lcd;
extern RTC_DS3231        rtc;
extern Preferences       prefs;
extern WiFiServer        transferServer;

// ── Config struct ─────────────────────────────────────────
struct Config {
  String  wifiSSID      = "";
  String  wifiPass      = "";
  String  aiIP          = "";
  int     aiPort        = 8081;
  String  aiModel       = "";
  String  aiSysPrompt   = "You are a helpful assistant.";
  uint8_t aiDispMode    = 0;
  uint8_t quranTrans    = 0;
  bool    soundEnabled  = true;
  uint8_t brightness    = 200;
  uint8_t theme         = 0;
  String  appsPath      = "/apps/";
  int     wifiTimeout   = 10;
};
extern Config cfg;

// ── App modes ─────────────────────────────────────────────
enum AppMode {
  MODE_CLOCK, MODE_TERMINAL, MODE_QURAN,
  MODE_AI, MODE_FILES, MODE_BROWSER,
  MODE_MEDIA, MODE_ALARM, MODE_TRANSFER,
  MODE_LAUNCHER, MODE_SETTINGS
};
extern AppMode currentMode;

// ── Shared state ──────────────────────────────────────────
extern bool sdReady;
extern bool rtcReady;
extern bool wifiReady;
extern bool appRunning;

// ── Glyph table ───────────────────────────────────────────
struct GlyphEntry { uint32_t cp; uint8_t bmp[CHAR_H]; };
extern GlyphEntry glyphTable[MAX_CHARS];
extern uint16_t   glyphCount;

// ── Forward declarations (01_display.ino) ────────────────
void oledPrint(int16_t x, int16_t y, const String& s, uint16_t color = WHITE);
void oledHeader(const String& title);
void lcdPrint(uint8_t row, const String& text);
void lcdClear();
void rgbSet(uint8_t r, uint8_t g, uint8_t b);
void rgbOff();    void rgbRed();    void rgbGreen();  void rgbBlue();
void rgbCyan();   void rgbYellow(); void rgbPurple(); void rgbWhite(); void rgbOrange();
void buzz(uint32_t freq, uint32_t ms);
void bootBeep();  void keyBeep();   void errorBeep(); void okBeep();  void alertBeep();
char readKeyboard();
bool connectWiFi();
bool loadFont();
void saveSettings();
void loadSettings();
