// ═══════════════════════════════════════════════════════════
//  apps_settings.ino - All settings, user configurable
// ═══════════════════════════════════════════════════════════

const char* settingItems[] = {
  "WiFi SSID",       // 0
  "WiFi Password",   // 1
  "AI Server IP",    // 2
  "AI Port",         // 3
  "AI Model",        // 4
  "AI Sys Prompt",   // 5
  "AI Display Mode", // 6  toggle
  "Quran Trans",     // 7  toggle
  "Sound",           // 8  toggle
  "Brightness",      // 9  +/-
  "Theme",           // 10 toggle
  "Apps Path",       // 11
  "WiFi Timeout",    // 12
  "Transfer Port",   // 13 info
  "Set Time",        // 14 HH:MM:SS
  "Set Date",        // 15 YYYY-MM-DD
  "RGB Test",        // 16 action
  "Connect WiFi",    // 17 action
  "Clear AI Hist",   // 18 action
  "Format SD",       // 19 action (dangerous)
  "Reboot",          // 20 action
  "About"            // 21 info
};
const int settingCount = 22;
int  settingSelected   = 0;
bool settingEditing    = false;
String settingEditBuf  = "";

String getSettingValue(int idx) {
  switch(idx) {
    case 0:  return cfg.wifiSSID.length()>0?cfg.wifiSSID:"(not set)";
    case 1:  return cfg.wifiPass.length()>0?"****":"(not set)";
    case 2:  return cfg.aiIP.length()>0?cfg.aiIP:"(not set)";
    case 3:  return String(cfg.aiPort);
    case 4:  return cfg.aiModel.length()>0?cfg.aiModel:"(not set)";
    case 5:  return cfg.aiSysPrompt.substring(0,10)+"..";
    case 6:  return cfg.aiDispMode==0?"Live":cfg.aiDispMode==1?"Full":"LCD";
    case 7:  return cfg.quranTrans==0?"EN":cfg.quranTrans==1?"KZ":cfg.quranTrans==2?"JL":"All";
    case 8:  return cfg.soundEnabled?"ON":"OFF";
    case 9:  return String(cfg.brightness);
    case 10: return cfg.theme==0?"Normal":"Inverted";
    case 11: return cfg.appsPath;
    case 12: return String(cfg.wifiTimeout)+"s";
    case 13: return String(TRANSFER_PORT);
    case 14: return "HH:MM:SS";
    case 15: return "YYYY-MM-DD";
    case 16: return ">> RGB TEST";
    case 17: return wifiReady?"Connected":"Connect Now";
    case 18: return "Clear History";
    case 19: return "!! DANGER !!";
    case 20: return "Reboot";
    case 21: return "ESP32 MiniPC v3";
    default: return "";
  }
}

bool isToggle(int idx) {
  return idx==6||idx==7||idx==8||idx==10;
}
bool isAction(int idx) {
  return idx==16||idx==17||idx==18||idx==19||idx==20;
}
bool isPlusMinus(int idx) {
  return idx==9;
}

void settingToggle(int idx) {
  switch(idx) {
    case 6:  cfg.aiDispMode=(cfg.aiDispMode+1)%3; break;
    case 7:  cfg.quranTrans=(cfg.quranTrans+1)%4; break;
    case 8:  cfg.soundEnabled=!cfg.soundEnabled; break;
    case 10: cfg.theme=(cfg.theme+1)%2; break;
  }
  saveSettings();
}

void settingDoAction(int idx) {
  switch(idx) {
    case 16: // RGB test
      rgbRed(); delay(300); rgbGreen(); delay(300);
      rgbBlue(); delay(300); rgbWhite(); delay(300); rgbOff();
      break;
    case 17: // Connect WiFi
      lcdPrint(0,"Connecting...   ");
      if(connectWiFi()) {
        lcdPrint(1,WiFi.localIP().toString());
        okBeep();
      } else { lcdPrint(1,"Failed!         "); errorBeep(); }
      delay(1500);
      break;
    case 18: // Clear AI history
      aiHistory="";
      aiAdd("History cleared");
      okBeep();
      break;
    case 19: // Format SD - double confirm
      oledHeader("FORMAT SD?!");
      oledPrint(0,20,"ALL DATA LOST!");
      oledPrint(0,32,"Hold F+ENTER 3s");
      oled.display();
      delay(3000);
      break;
    case 20: // Reboot
      lcdPrint(0,"Rebooting...    ");
      delay(500); ESP.restart();
      break;
  }
}

void applySettingEdit(int idx) {
  String v=settingEditBuf;
  switch(idx) {
    case 0:  cfg.wifiSSID   =v; break;
    case 1:  cfg.wifiPass   =v; break;
    case 2:  cfg.aiIP       =v; break;
    case 3:  cfg.aiPort     =v.toInt(); break;
    case 4:  cfg.aiModel    =v; break;
    case 5:  cfg.aiSysPrompt=v; break;
    case 11: cfg.appsPath   =v; break;
    case 12: cfg.wifiTimeout=v.toInt(); break;
    case 14: { // Set time
      int h=v.substring(0,2).toInt();
      int m=v.substring(3,5).toInt();
      int s=v.substring(6,8).toInt();
      if(rtcReady) {
        DateTime now=rtc.now();
        rtc.adjust(DateTime(now.year(),now.month(),now.day(),h,m,s));
      }
      break;
    }
    case 15: { // Set date
      int y=v.substring(0,4).toInt();
      int mo=v.substring(5,7).toInt();
      int d=v.substring(8,10).toInt();
      if(rtcReady) {
        DateTime now=rtc.now();
        rtc.adjust(DateTime(y,mo,d,now.hour(),now.minute(),now.second()));
      }
      break;
    }
    default: break;
  }
  saveSettings(); okBeep();
}

void drawSettings() {
  oledHeader("> SETTINGS");
  int start=(settingSelected/3)*3;
  for(int i=0;i<3&&(start+i)<settingCount;i++) {
    int idx=start+i;
    bool sel=(idx==settingSelected);
    String prefix=sel?(settingEditing?"[":">"):" ";
    String val=getSettingValue(idx);
    String line=prefix+String(settingItems[idx]).substring(0,7)+":"+val;
    oledPrint(0,12+i*16,line.substring(0,16));
  }
  if(settingEditing) {
    oled.drawFastHLine(0,52,OLED_W,WHITE);
    oledPrint(0,54,"Edit:"+settingEditBuf.substring(0,10)+"_");
  } else {
    oledPrint(0,56,"ENT=edit +/-=chg W/S=nav");
  }
  oled.display();
  lcdPrint(0,String(settingItems[settingSelected]).substring(0,16));
  lcdPrint(1,getSettingValue(settingSelected).substring(0,16));
}

void handleSettingsKey(char key) {
  if(settingEditing) {
    if(key=='\n'||key=='\r') {
      applySettingEdit(settingSelected);
      settingEditing=false; settingEditBuf="";
    } else if(key==27) { settingEditing=false; settingEditBuf=""; }
    else if((key==8||key==127)&&settingEditBuf.length()>0)
      settingEditBuf.remove(settingEditBuf.length()-1);
    else if(settingEditBuf.length()<40) settingEditBuf+=key;
  } else {
    if(key=='w'||key=='W') { if(settingSelected>0) settingSelected--; }
    else if(key=='s'||key=='S') { if(settingSelected<settingCount-1) settingSelected++; }
    else if(key=='\n'||key=='\r') {
      if(isToggle(settingSelected)) settingToggle(settingSelected);
      else if(isAction(settingSelected)) settingDoAction(settingSelected);
      else if(isPlusMinus(settingSelected)) {
        cfg.brightness=min(255,cfg.brightness+10);
        oled.ssd1306_command(SSD1306_SETCONTRAST);
        oled.ssd1306_command(cfg.brightness);
        saveSettings();
      } else {
        settingEditing=true;
        String cur=getSettingValue(settingSelected);
        settingEditBuf=(cur=="(not set)"||cur=="****")?"":cur;
      }
    }
    else if(key=='+'||key=='d'||key=='D') {
      if(isPlusMinus(settingSelected)) {
        cfg.brightness=min(255,cfg.brightness+10);
        oled.ssd1306_command(SSD1306_SETCONTRAST);
        oled.ssd1306_command(cfg.brightness);
        saveSettings();
      } else if(isToggle(settingSelected)) settingToggle(settingSelected);
    }
    else if(key=='-'||key=='a'||key=='A') {
      if(isPlusMinus(settingSelected)) {
        cfg.brightness=max(10,cfg.brightness-10);
        oled.ssd1306_command(SSD1306_SETCONTRAST);
        oled.ssd1306_command(cfg.brightness);
        saveSettings();
      }
    }
    else if(key==27) currentMode=MODE_CLOCK;
  }
}
