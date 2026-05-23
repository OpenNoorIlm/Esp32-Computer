// ═══════════════════════════════════════════════════════════
//  apps_alarm.ino - Alarm, Timer, Stopwatch
// ═══════════════════════════════════════════════════════════

// ── Alarms ────────────────────────────────────────────────
struct Alarm {
  uint8_t hour, minute;
  bool    active;
  bool    repeat; // daily
  String  label;
};
Alarm alarms[8];
int   alarmCount    = 0;
int   alarmSelected = 0;
bool  alarmFiring   = false;

// ── Timer ─────────────────────────────────────────────────
uint32_t timerDuration  = 0; // ms
uint32_t timerStartMs   = 0;
bool     timerRunning   = false;
bool     timerFinished  = false;
String   timerInput     = ""; // HH:MM:SS input

// ── Stopwatch ─────────────────────────────────────────────
uint32_t swStart    = 0;
uint32_t swElapsed  = 0;
bool     swRunning  = false;
uint32_t swLaps[8];
int      swLapCount = 0;

enum AlarmSubMode { AL_MENU, AL_LIST, AL_EDIT, AL_TIMER, AL_STOPWATCH, AL_FIRING };
AlarmSubMode alarmSubMode = AL_MENU;

bool alarmEditing    = false;
String alarmEditBuf  = "";
int   alarmEditField = 0; // 0=hour 1=min 2=label

void alarmSaveToSD() {
  if(!sdReady) return;
  File f=SD.open("/alarms.json",FILE_WRITE);
  if(!f) return;
  DynamicJsonDocument doc(1024);
  JsonArray arr=doc.createNestedArray("alarms");
  for(int i=0;i<alarmCount;i++) {
    JsonObject a=arr.createNestedObject();
    a["h"]=alarms[i].hour; a["m"]=alarms[i].minute;
    a["active"]=alarms[i].active; a["repeat"]=alarms[i].repeat;
    a["label"]=alarms[i].label;
  }
  serializeJson(doc,f); f.close();
}

void alarmLoadFromSD() {
  if(!sdReady) return;
  File f=SD.open("/alarms.json");
  if(!f) return;
  DynamicJsonDocument doc(1024);
  if(deserializeJson(doc,f)) { f.close(); return; }
  f.close();
  alarmCount=0;
  JsonArray arr=doc["alarms"].as<JsonArray>();
  for(JsonObject a:arr) {
    if(alarmCount>=8) break;
    alarms[alarmCount].hour    = a["h"].as<uint8_t>();
    alarms[alarmCount].minute  = a["m"].as<uint8_t>();
    alarms[alarmCount].active  = a["active"].as<bool>();
    alarms[alarmCount].repeat  = a["repeat"].as<bool>();
    alarms[alarmCount].label   = a["label"].as<String>();
    alarmCount++;
  }
}

void alarmCheck() {
  if(!rtcReady||alarmFiring) return;
  DateTime now=rtc.now();
  for(int i=0;i<alarmCount;i++) {
    if(alarms[i].active &&
       alarms[i].hour==now.hour() &&
       alarms[i].minute==now.minute() &&
       now.second()==0) {
      alarmFiring=true;
      alarmSelected=i;
      alarmSubMode=AL_FIRING;
      currentMode=MODE_ALARM;
      if(!alarms[i].repeat) alarms[i].active=false;
      alarmSaveToSD();
    }
  }
}

String formatTime(uint32_t ms) {
  uint32_t s=ms/1000;
  uint32_t m=s/60; s=s%60;
  uint32_t h=m/60; m=m%60;
  char buf[9];
  snprintf(buf,sizeof(buf),"%02d:%02d:%02d",h,m,s);
  return String(buf);
}

void drawAlarmMenu() {
  oledHeader("> ALARM/TIMER");
  oledPrint(0,14,"1. Alarms");
  oledPrint(0,26,"2. Timer");
  oledPrint(0,38,"3. Stopwatch");
  oledPrint(0,56,"1/2/3=select ESC=back");
  oled.display();
  lcdPrint(0,"ALARM/TIMER/SW  ");
  lcdPrint(1,"1=alm 2=tmr 3=sw");
}

void drawAlarmList() {
  oledHeader("ALARMS ("+String(alarmCount)+"/8)");
  for(int i=0;i<min(alarmCount,4);i++) {
    bool sel=(i==alarmSelected);
    char buf[12];
    snprintf(buf,sizeof(buf),"%02d:%02d",alarms[i].hour,alarms[i].minute);
    String line=String(sel?">":" ")+(alarms[i].active?"*":" ")+
                String(buf)+" "+alarms[i].label.substring(0,6);
    oledPrint(0,12+i*12,line.substring(0,16));
  }
  oledPrint(0,56,"N=new DEL=del ENTER=tog");
  oled.display();
  lcdPrint(0,"ALARMS          ");
  if(alarmCount>0) {
    char buf[12];
    snprintf(buf,sizeof(buf),"%02d:%02d",alarms[alarmSelected].hour,alarms[alarmSelected].minute);
    lcdPrint(1,String(buf)+" "+alarms[alarmSelected].label.substring(0,9));
  } else lcdPrint(1,"No alarms set   ");
}

void drawAlarmEdit() {
  oledHeader(alarmEditing?"EDIT ALARM":"NEW ALARM");
  String fields[3]={"Hour:","Min: ","Label:"};
  for(int i=0;i<3;i++) {
    String val="";
    if(i==0) val=alarmEditField==0?alarmEditBuf:String(alarms[alarmSelected].hour);
    else if(i==1) val=alarmEditField==1?alarmEditBuf:String(alarms[alarmSelected].minute);
    else val=alarmEditField==2?alarmEditBuf:alarms[alarmSelected].label;
    oledPrint(0,12+i*14,fields[i]+(alarmEditField==i?val+"_":val));
  }
  oledPrint(0,56,"TAB=next ENTER=save");
  oled.display();
  lcdPrint(0,"SET ALARM       ");
  lcdPrint(1,fields[alarmEditField]);
}

void drawTimer() {
  oledHeader("> TIMER");
  if(!timerRunning&&!timerFinished) {
    oledPrint(0,16,"Set time HH:MM:SS:");
    oledPrint(0,32,timerInput+"_");
    oledPrint(0,48,"ENTER=start ESC=back");
  } else if(timerRunning) {
    uint32_t elapsed=millis()-timerStartMs;
    uint32_t remaining=timerDuration>elapsed?timerDuration-elapsed:0;
    if(remaining==0) { timerFinished=true; timerRunning=false; alertBeep(); }
    oledPrint(0,20,"Remaining:");
    oledPrint(20,36,formatTime(remaining));
    // Progress bar
    int progress=(int)((timerDuration-remaining)*100/max(1UL,timerDuration));
    oled.drawRect(4,48,120,8,WHITE);
    oled.fillRect(4,48,progress*120/100,8,WHITE);
    oledPrint(0,58,"SPC=pause ESC=stop");
  } else {
    oledPrint(0,24,"TIMER DONE!");
    oledPrint(20,38,formatTime(timerDuration));
    oledPrint(0,52,"ENTER=reset ESC=back");
    alertBeep();
  }
  oled.display();
  lcdPrint(0,"TIMER           ");
  if(timerRunning) {
    uint32_t rem=timerDuration-(millis()-timerStartMs);
    lcdPrint(1,formatTime(rem));
  } else lcdPrint(1,timerFinished?"DONE!":"Set time...     ");
}

void drawStopwatch() {
  oledHeader("> STOPWATCH");
  uint32_t elapsed=swRunning?(millis()-swStart+swElapsed):swElapsed;
  oled.setTextSize(2); oled.setCursor(4,14); oled.setTextColor(WHITE);
  oled.print(formatTime(elapsed)); oled.setTextSize(1);
  // Show last 2 laps
  for(int i=max(0,swLapCount-2);i<swLapCount;i++) {
    oledPrint(0,38+(i-(max(0,swLapCount-2)))*12,
              "L"+String(i+1)+":"+formatTime(swLaps[i]));
  }
  oledPrint(0,56,"SPC=start/stop L=lap R=rst");
  oled.display();
  lcdPrint(0,swRunning?"STOPWATCH RUN   ":"STOPWATCH STOP  ");
  lcdPrint(1,formatTime(elapsed));
}

void drawAlarmFiring() {
  oled.clearDisplay();
  oled.fillRect(0,0,OLED_W,OLED_H,WHITE);
  oledPrint(20,4,"!! ALARM !!",BLACK);
  char buf[9];
  snprintf(buf,sizeof(buf),"%02d:%02d",alarms[alarmSelected].hour,alarms[alarmSelected].minute);
  oledPrint(24,20,String(buf),BLACK);
  oledPrint(4,36,alarms[alarmSelected].label.substring(0,16),BLACK);
  oledPrint(16,52,"ANY KEY = DISMISS",BLACK);
  oled.display();
  alertBeep();
  lcdPrint(0,"!! ALARM !!     ");
  lcdPrint(1,alarms[alarmSelected].label.substring(0,16));
}

void drawAlarm() {
  switch(alarmSubMode) {
    case AL_MENU:      drawAlarmMenu();  break;
    case AL_LIST:      drawAlarmList();  break;
    case AL_EDIT:      drawAlarmEdit();  break;
    case AL_TIMER:     drawTimer();      break;
    case AL_STOPWATCH: drawStopwatch();  break;
    case AL_FIRING:    drawAlarmFiring();break;
  }
}

void handleAlarmKey(char key) {
  switch(alarmSubMode) {
    case AL_MENU:
      if(key=='1') alarmSubMode=AL_LIST;
      else if(key=='2') { timerInput=""; timerRunning=false; timerFinished=false; alarmSubMode=AL_TIMER; }
      else if(key=='3') alarmSubMode=AL_STOPWATCH;
      else if(key==27) currentMode=MODE_CLOCK;
      break;

    case AL_LIST:
      if(key=='w'||key=='W') { if(alarmSelected>0) alarmSelected--; }
      else if(key=='s'||key=='S') { if(alarmSelected<alarmCount-1) alarmSelected++; }
      else if(key=='\n'||key=='\r') {
        if(alarmCount>0) alarms[alarmSelected].active=!alarms[alarmSelected].active;
        alarmSaveToSD();
      }
      else if(key=='n'||key=='N') {
        if(alarmCount<8) {
          alarms[alarmCount]={0,0,true,false,"Alarm"};
          alarmSelected=alarmCount++;
          alarmEditField=0; alarmEditBuf="";
          alarmSubMode=AL_EDIT;
        }
      }
      else if(key==127||key==8) {
        if(alarmCount>0) {
          for(int i=alarmSelected;i<alarmCount-1;i++) alarms[i]=alarms[i+1];
          alarmCount--;
          if(alarmSelected>=alarmCount) alarmSelected=max(0,alarmCount-1);
          alarmSaveToSD();
        }
      }
      else if(key==27) alarmSubMode=AL_MENU;
      break;

    case AL_EDIT:
      if(key=='\t') {
        // Save current field
        if(alarmEditField==0) alarms[alarmSelected].hour=constrain(alarmEditBuf.toInt(),0,23);
        else if(alarmEditField==1) alarms[alarmSelected].minute=constrain(alarmEditBuf.toInt(),0,59);
        else if(alarmEditField==2) alarms[alarmSelected].label=alarmEditBuf;
        alarmEditBuf="";
        alarmEditField=(alarmEditField+1)%3;
      } else if(key=='\n'||key=='\r') {
        if(alarmEditField==2) alarms[alarmSelected].label=alarmEditBuf;
        alarmSaveToSD(); alarmSubMode=AL_LIST;
      } else if((key==8||key==127)&&alarmEditBuf.length()>0) {
        alarmEditBuf.remove(alarmEditBuf.length()-1);
      } else if(alarmEditBuf.length()<12) alarmEditBuf+=key;
      break;

    case AL_TIMER:
      if(!timerRunning&&!timerFinished) {
        if(key=='\n'||key=='\r') {
          // Parse HH:MM:SS
          int h=timerInput.substring(0,2).toInt();
          int m=timerInput.substring(3,5).toInt();
          int s=timerInput.substring(6,8).toInt();
          timerDuration=((uint32_t)h*3600+m*60+s)*1000;
          if(timerDuration>0) { timerStartMs=millis(); timerRunning=true; }
        } else if(key==27) alarmSubMode=AL_MENU;
        else if((key==8||key==127)&&timerInput.length()>0)
          timerInput.remove(timerInput.length()-1);
        else if(timerInput.length()<8) timerInput+=key;
      } else if(timerRunning) {
        if(key==' ') { swElapsed=millis()-timerStartMs; timerRunning=false; }
        else if(key==27) { timerRunning=false; timerFinished=false; timerInput=""; }
      } else {
        if(key=='\n'||key=='\r') { timerFinished=false; timerInput=""; }
        else if(key==27) alarmSubMode=AL_MENU;
      }
      break;

    case AL_STOPWATCH:
      if(key==' ') {
        if(swRunning) { swElapsed+=millis()-swStart; swRunning=false; }
        else { swStart=millis(); swRunning=true; }
      } else if(key=='l'||key=='L') {
        if(swLapCount<8) swLaps[swLapCount++]=(swRunning?millis()-swStart:0)+swElapsed;
      } else if(key=='r'||key=='R') {
        swRunning=false; swElapsed=0; swLapCount=0;
      } else if(key==27) alarmSubMode=AL_MENU;
      break;

    case AL_FIRING:
      // Any key dismisses
      alarmFiring=false;
      noTone(BUZZER_PIN);
      alarmSubMode=AL_MENU;
      break;
  }
}
