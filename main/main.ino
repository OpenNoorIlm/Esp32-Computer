// ═══════════════════════════════════════════════════════════
//  main.ino - Setup, loop, key handler, boot screen
// ═══════════════════════════════════════════════════════════

#include "config.h"

// Forward declarations
void drawClock();
void drawTerminal();    void handleTerminalKey(char k);
void drawQuran();       void handleQuranKey(char k);
void drawAIChat();      void handleAIKey(char k);
void drawFiles();       void handleFilesKey(char k);
void drawBrowser();     void handleBrowserKey(char k);
void drawMedia();       void handleMediaKey(char k);
void drawAlarm();       void handleAlarmKey(char k);
void drawTransfer();    void handleTransferKey(char k);
void drawLauncher();    void handleLauncherKey(char k);
void drawSettings();    void handleSettingsKey(char k);
void mediaPlayNote();
void transferLoop();
void alarmCheck();
void loadAppList();
bool loadFont();
void initLua();
bool connectWiFi();
void alarmLoadFromSD();
void mediaLoadList();
void fileLoadDir(const String&);
void termAdd(const String&);
void termProcess(const String&);
void aiAdd(const String&);
void okBeep();
void errorBeep();
void rgbBlue(); void rgbOff(); void rgbGreen(); void rgbCyan();
void rgbPurple(); void rgbWhite(); void rgbOrange(); void rgbRed(); void rgbYellow();
void bootBeep();
void keyBeep();

extern String aiHistory;
extern String aiLines[4];
extern bool   aiWaiting;
extern bool   transferServerOn;
extern String termLines[4];
extern String termInput;

void handleKey(char key) {
  if(!key) return;
  keyBeep();

  if(!appRunning) {
    if(key==0x01){ currentMode=MODE_CLOCK;    rgbBlue();   return; }
    if(key==0x02){ currentMode=MODE_TERMINAL; rgbCyan();   return; }
    if(key==0x03){ currentMode=MODE_QURAN;    rgbGreen();  return; }
    if(key==0x04){ currentMode=MODE_AI;       rgbPurple(); return; }
    if(key==0x05){ currentMode=MODE_FILES;    rgbOrange(); fileLoadDir("/"); return; }
    if(key==0x06){ currentMode=MODE_BROWSER;  rgbWhite();  return; }
    if(key==0x07){ currentMode=MODE_MEDIA;    rgbYellow(); mediaLoadList(); return; }
    if(key==0x08){ currentMode=MODE_ALARM;    rgbRed();    return; }
    if(key==0x09){ currentMode=MODE_TRANSFER; rgbCyan();   return; }
    if(key==0x0A){ currentMode=MODE_LAUNCHER; rgbWhite();  loadAppList(); return; }
    if(key==0x0B){ currentMode=MODE_SETTINGS; rgbYellow(); return; }
  }

  switch(currentMode) {
    case MODE_TERMINAL: handleTerminalKey(key); break;
    case MODE_QURAN:    handleQuranKey(key);    break;
    case MODE_AI:       handleAIKey(key);       break;
    case MODE_FILES:    handleFilesKey(key);    break;
    case MODE_BROWSER:  handleBrowserKey(key);  break;
    case MODE_MEDIA:    handleMediaKey(key);    break;
    case MODE_ALARM:    handleAlarmKey(key);    break;
    case MODE_TRANSFER: handleTransferKey(key); break;
    case MODE_LAUNCHER: handleLauncherKey(key); break;
    case MODE_SETTINGS: handleSettingsKey(key); break;
    default: break;
  }
}

void handleTerminalKey(char key) {
  if(key=='\n'||key=='\r') {
    termAdd("$"+termInput); termProcess(termInput); termInput="";
  } else if(key==8||key==127) {
    if(termInput.length()>0) termInput.remove(termInput.length()-1);
  } else if(termInput.length()<14) termInput+=key;
}

void showBootScreen() {
  oled.clearDisplay();
  oledPrint(16,4, "ESP32  MiniPC");
  oledPrint(28,16,"   v3.0");
  oledPrint(4,28, "Lua+AI+Quran+More");
  for(int i=0;i<=100;i+=4) {
    oled.drawRect(4,40,120,10,WHITE);
    oled.fillRect(4,40,(int)(120*i/100.0),10,WHITE);
    oled.display(); delay(25);
  }
  oledPrint(32,54,"Ready!");
  oled.display(); delay(400);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n[BOOT] ESP32 MiniPC v3");

  pinMode(BUZZER_PIN,OUTPUT);
  pinMode(RGB_R,OUTPUT);
  pinMode(RGB_G,OUTPUT);
  pinMode(RGB_B,OUTPUT);
  rgbRed();

  Wire.begin(21,22);
  loadSettings();

  if(!oled.begin(SSD1306_SWITCHCAPVCC,OLED_ADDR))
    Serial.println("[OLED] FAILED");
  oled.ssd1306_command(SSD1306_SETCONTRAST);
  oled.ssd1306_command(cfg.brightness);

  lcd.init(); lcd.backlight();
  lcdPrint(0,"ESP32 MiniPC v3 ");
  lcdPrint(1,"Booting...      ");

  if(!rtc.begin()) {
    Serial.println("[RTC] FAILED"); rtcReady=false;
  } else {
    rtcReady=true;
    if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
    Serial.println("[RTC] OK");
  }

  if(!SD.begin(SD_CS)) {
    Serial.println("[SD] FAILED"); sdReady=false;
  } else {
    sdReady=true;
    Serial.println("[SD] OK");
    loadFont();
    loadAppList();
    alarmLoadFromSD();
    mediaLoadList();
    if(!SD.exists("/apps"))  SD.mkdir("/apps");
    if(!SD.exists("/music")) SD.mkdir("/music");
  }

  if(cfg.wifiSSID.length()>0) {
    lcdPrint(1,"WiFi connecting ");
    if(connectWiFi()) {
      Serial.println("[WiFi] "+WiFi.localIP().toString());
      transferServer.begin();
      transferServerOn=true;
    }
  }

  initLua();
  Serial.println("[LUA] OK");

  for(int i=0;i<4;i++) termLines[i]="";
  termAdd("ESP32 MiniPC v3");
  termAdd("11 apps + Lua");
  termAdd("Type 'help'");
  for(int i=0;i<4;i++) aiLines[i]="";
  aiAdd("AI Chat Ready");
  aiAdd(wifiReady?"IP:"+WiFi.localIP().toString():"Set WiFi in Settings");

  fileLoadDir("/");

  showBootScreen();
  bootBeep();
  rgbGreen();
  Serial.println("[BOOT] Done!");
}

uint32_t lastDraw=0, lastKey=0, lastBeat=0, lastAlarm=0;

void loop() {
  uint32_t now=millis();

  if(now-lastKey>=50) {
    lastKey=now;
    char k=readKeyboard();
    if(k) handleKey(k);
  }

  if(!appRunning && now-lastDraw>=250) {
    lastDraw=now;
    switch(currentMode) {
      case MODE_CLOCK:    drawClock();    break;
      case MODE_TERMINAL: drawTerminal(); break;
      case MODE_QURAN:    drawQuran();    break;
      case MODE_AI:       drawAIChat();   break;
      case MODE_FILES:    drawFiles();    break;
      case MODE_BROWSER:  drawBrowser();  break;
      case MODE_MEDIA:    drawMedia();    break;
      case MODE_ALARM:    drawAlarm();    break;
      case MODE_TRANSFER: drawTransfer(); break;
      case MODE_LAUNCHER: drawLauncher(); break;
      case MODE_SETTINGS: drawSettings(); break;
    }
  }

  mediaPlayNote();
  transferLoop();

  if(now-lastAlarm>=5000) {
    lastAlarm=now;
    alarmCheck();
  }

  if(currentMode==MODE_CLOCK && now-lastBeat>=1000) {
    lastBeat=now;
    static bool beat=false;
    beat=!beat;
    if(beat) rgbBlue(); else rgbOff();
  }
}