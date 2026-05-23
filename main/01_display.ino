// ═══════════════════════════════════════════════════════════
//  display.ino - OLED/LCD helpers, font loader, RGB, Buzzer
// ═══════════════════════════════════════════════════════════

// ── Font loader ───────────────────────────────────────────
bool loadFont() {
  if (!sdReady) return false;
  File f = SD.open("/char.bitmap");
  if (!f) return false;
  char magic[4]; f.read((uint8_t*)magic, 4);
  if (strncmp(magic, "CMAP", 4) != 0) { f.close(); return false; }
  f.read(); f.read(); f.read();
  uint8_t cb[2]; f.read(cb, 2);
  uint16_t count = cb[0] | (cb[1] << 8);
  glyphCount = 0;
  for (uint16_t i = 0; i < count && i < MAX_CHARS; i++) {
    uint8_t cpb[4]; f.read(cpb, 4);
    glyphTable[i].cp = cpb[0]|(cpb[1]<<8)|(cpb[2]<<16)|(cpb[3]<<24);
    f.read(glyphTable[i].bmp, CHAR_H);
    glyphCount++;
  }
  f.close();
  return true;
}

int findGlyph(uint32_t cp) {
  for (uint16_t i = 0; i < glyphCount; i++)
    if (glyphTable[i].cp == cp) return i;
  return -1;
}

void oledDrawChar(int16_t x, int16_t y, uint32_t cp, uint16_t color) {
  int idx = findGlyph(cp);
  if (idx < 0) { oled.drawRect(x, y, CHAR_W-1, CHAR_H-1, color); return; }
  for (uint8_t row = 0; row < CHAR_H; row++) {
    uint8_t b = glyphTable[idx].bmp[row];
    for (uint8_t col = 0; col < CHAR_W; col++)
      if (b & (1 << (7-col))) oled.drawPixel(x+col, y+row, color);
  }
}

void oledPrint(int16_t x, int16_t y, const String& s, uint16_t color) {
  int16_t cx = x;
  for (uint16_t i = 0; i < s.length(); i++) {
    if (cx + CHAR_W > OLED_W) break;
    oledDrawChar(cx, y, (uint32_t)s[i], color);
    cx += CHAR_W;
  }
}

void oledHeader(const String& title) {
  oled.clearDisplay();
  if (cfg.theme == 1) oled.fillRect(0, 0, OLED_W, 10, WHITE);
  uint16_t tc = (cfg.theme == 1) ? BLACK : WHITE;
  oledPrint(2, 1, title, tc);
  oled.drawFastHLine(0, 10, OLED_W, WHITE);
}

// ── LCD ───────────────────────────────────────────────────
void lcdPrint(uint8_t row, const String& text) {
  lcd.setCursor(0, row);
  String p = text;
  while (p.length() < 16) p += ' ';
  lcd.print(p.substring(0, 16));
}
void lcdClear() { lcdPrint(0, ""); lcdPrint(1, ""); }

// ── RGB ───────────────────────────────────────────────────
void rgbSet(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RGB_R, r); analogWrite(RGB_G, g); analogWrite(RGB_B, b);
}
void rgbOff()    { rgbSet(0,0,0); }
void rgbRed()    { rgbSet(255,0,0); }
void rgbGreen()  { rgbSet(0,255,0); }
void rgbBlue()   { rgbSet(0,0,255); }
void rgbCyan()   { rgbSet(0,255,255); }
void rgbYellow() { rgbSet(255,200,0); }
void rgbPurple() { rgbSet(128,0,255); }
void rgbWhite()  { rgbSet(255,255,255); }
void rgbOrange() { rgbSet(255,80,0); }

// ── Buzzer ────────────────────────────────────────────────
void buzz(uint32_t freq, uint32_t ms) {
  if (!cfg.soundEnabled) return;
  tone(BUZZER_PIN, freq, ms); delay(ms + 10);
}
void bootBeep()  { buzz(523,80); buzz(659,80); buzz(784,120); }
void keyBeep()   { buzz(1000,15); }
void errorBeep() { buzz(200,300); buzz(150,300); }
void okBeep()    { buzz(880,80); buzz(1046,120); }
void alertBeep() { buzz(880,200); buzz(880,200); buzz(880,200); }

// ── Keyboard ─────────────────────────────────────────────
char readKeyboard() {
  Wire.requestFrom(KB_ADDR, 1);
  if (Wire.available()) {
    uint8_t k = Wire.read();
    if (k != 0x00 && k != 0xFF) return (char)k;
  }
  return 0;
}

// ── WiFi ─────────────────────────────────────────────────
bool connectWiFi() {
  if (cfg.wifiSSID.length() == 0) return false;
  WiFi.begin(cfg.wifiSSID.c_str(), cfg.wifiPass.c_str());
  lcdPrint(0, "WiFi Connecting ");
  lcdPrint(1, cfg.wifiSSID.substring(0,16));
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < cfg.wifiTimeout * 2) {
    delay(500); tries++;
  }
  wifiReady = (WiFi.status() == WL_CONNECTED);
  return wifiReady;
}

// ── String helpers ────────────────────────────────────────
String wrapLine(const String& text, int maxChars) {
  if ((int)text.length() <= maxChars) return text;
  int lastSpace = text.lastIndexOf(' ', maxChars);
  if (lastSpace < 0) return text.substring(0, maxChars);
  return text.substring(0, lastSpace);
}

String stripHTML(String html) {
  String out = "";
  bool inTag = false;
  for (int i = 0; i < (int)html.length(); i++) {
    char c = html[i];
    if (c == '<') { inTag = true; continue; }
    if (c == '>') { inTag = false; out += ' '; continue; }
    if (!inTag) out += c;
  }
  // Collapse multiple spaces
  String clean = "";
  bool lastSpace = false;
  for (int i = 0; i < (int)out.length(); i++) {
    if (out[i] == ' ' || out[i] == '\n' || out[i] == '\r' || out[i] == '\t') {
      if (!lastSpace) { clean += ' '; lastSpace = true; }
    } else { clean += out[i]; lastSpace = false; }
  }
  return clean;
}
