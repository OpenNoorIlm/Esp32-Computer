// ═══════════════════════════════════════════════════════════
//  apps_media.ino - Media player (buzzer + metadata + HTTP)
// ═══════════════════════════════════════════════════════════

struct Track { String name; String artist; String album; int duration; };
Track  mediaList[20];
int    mediaCount    = 0;
int    mediaSelected = 0;
bool   mediaPlaying  = false;
bool   mediaPaused   = false;
int    mediaPosition = 0; // seconds
uint32_t mediaLastTick = 0;
String mediaHTTPBase = ""; // for HTTP control
enum MediaSubMode { MD_LIST, MD_PLAY, MD_HTTP };
MediaSubMode mediaSubMode = MD_LIST;

// Buzzer melody player
struct Note { int freq; int dur; };
Note currentMelody[64];
int  melodyLen    = 0;
int  melodyPos    = 0;
uint32_t noteEnd  = 0;

// Simple melody: Nokia tune as default
Note nokiaTune[] = {
  {1319,125},{988,125},{1175,125},{1397,125},
  {1568,125},{1319,125},{1047,125},{1047,125},
  {1175,125},{1319,125},{988,125},{784,250}
};

void mediaLoadList() {
  mediaCount=0; mediaSelected=0;
  if(!sdReady) return;
  File dir=SD.open("/music/");
  if(!dir) { SD.mkdir("/music/"); return; }
  while(mediaCount<20) {
    File f=dir.openNextFile(); if(!f) break;
    String name=String(f.name());
    if(name.endsWith(".txt")||name.endsWith(".mml")||name.endsWith(".mel")) {
      mediaList[mediaCount].name=name;
      mediaList[mediaCount].artist="Unknown";
      mediaList[mediaCount].album="SD Card";
      mediaList[mediaCount].duration=0;
      mediaCount++;
    }
    f.close();
  }
  dir.close();
}

void mediaLoadMelody(const String& filename) {
  if(!sdReady) return;
  String path="/music/"+filename;
  File f=SD.open(path.c_str());
  if(!f) return;
  melodyLen=0; melodyPos=0;
  // Format: freq,dur per line e.g. "440,250"
  while(f.available()&&melodyLen<64) {
    String line=f.readStringUntil('\n'); line.trim();
    int comma=line.indexOf(',');
    if(comma>0) {
      currentMelody[melodyLen].freq=line.substring(0,comma).toInt();
      currentMelody[melodyLen].dur=line.substring(comma+1).toInt();
      melodyLen++;
    }
  }
  f.close();
}

void mediaPlayNote() {
  if(!mediaPlaying||mediaPaused) return;
  uint32_t now=millis();
  if(now>=noteEnd&&melodyPos<melodyLen) {
    int freq=currentMelody[melodyPos].freq;
    int dur=currentMelody[melodyPos].dur;
    if(cfg.soundEnabled) tone(BUZZER_PIN,freq,dur);
    noteEnd=now+dur+20;
    melodyPos++;
    if(melodyPos>=melodyLen) {
      melodyPos=0; // loop
      mediaPosition++;
    }
  }
}

void mediaHTTPControl(const String& action) {
  if(!wifiReady||mediaHTTPBase.length()==0) return;
  String url=mediaHTTPBase+"/"+action;
  HTTPClient http; http.begin(url); http.GET(); http.end();
}

void drawMediaList() {
  oledHeader("> MEDIA PLAYER");
  if(mediaCount==0) {
    oledPrint(0,20,"No tracks found");
    oledPrint(0,32,"Add .mel files");
    oledPrint(0,44,"to /music/ on SD");
    oled.display();
    lcdPrint(0,"MEDIA PLAYER    ");
    lcdPrint(1,"No tracks found ");
    return;
  }
  int start=(mediaSelected/4)*4;
  for(int i=0;i<4&&(start+i)<mediaCount;i++) {
    int idx=start+i;
    bool sel=(idx==mediaSelected);
    oledPrint(0,12+i*12,(sel?">":" ")+mediaList[idx].name.substring(0,15));
  }
  oledPrint(0,56,"ENTER=play H=http");
  oled.display();
  lcdPrint(0,"MEDIA PLAYER    ");
  lcdPrint(1,mediaList[mediaSelected].name.substring(0,16));
}

void drawMediaPlay() {
  oledHeader(mediaList[mediaSelected].name.substring(0,14));
  oledPrint(0,14,mediaList[mediaSelected].artist);
  oledPrint(0,26,mediaList[mediaSelected].album);
  // Progress bar
  int barW=100;
  int filled=mediaPlaying?(melodyPos*barW/max(1,melodyLen)):0;
  oled.drawRect(14,38,barW,6,WHITE);
  oled.fillRect(14,38,filled,6,WHITE);
  String status=mediaPlaying?(mediaPaused?"||":">>"):"[]";
  oledPrint(0,48,status+" "+String(melodyPos)+"/"+String(melodyLen));
  oledPrint(0,56,"SPC=pause S=stop ESC=bk");
  oled.display();
  lcdPrint(0,mediaPlaying?(mediaPaused?"PAUSED":"PLAYING"):"STOPPED");
  lcdPrint(1,mediaList[mediaSelected].name.substring(0,16));
}

void drawMediaHTTP() {
  oledHeader("> HTTP CONTROL");
  oledPrint(0,14,"Server:");
  oledPrint(0,26,mediaHTTPBase.length()>0?mediaHTTPBase.substring(0,16):"Not set");
  oledPrint(0,40,"P=play S=stop N=next");
  oledPrint(0,52,"U=set URL ESC=back");
  oled.display();
  lcdPrint(0,"HTTP MEDIA CTRL ");
  lcdPrint(1,wifiReady?"WiFi OK         ":"No WiFi!        ");
}

void drawMedia() {
  switch(mediaSubMode) {
    case MD_LIST: drawMediaList(); break;
    case MD_PLAY: drawMediaPlay(); break;
    case MD_HTTP: drawMediaHTTP(); break;
  }
}

void handleMediaKey(char key) {
  switch(mediaSubMode) {
    case MD_LIST:
      if(key=='w'||key=='W') { if(mediaSelected>0) mediaSelected--; }
      else if(key=='s'||key=='S') { if(mediaSelected<mediaCount-1) mediaSelected++; }
      else if(key=='\n'||key=='\r') {
        if(mediaCount>0) {
          mediaLoadMelody(mediaList[mediaSelected].name);
          mediaPlaying=true; mediaPaused=false; melodyPos=0;
          mediaSubMode=MD_PLAY;
        }
      }
      else if(key=='h'||key=='H') mediaSubMode=MD_HTTP;
      else if(key==27) currentMode=MODE_CLOCK;
      break;

    case MD_PLAY:
      if(key==' ') { mediaPaused=!mediaPaused; if(!mediaPaused) noTone(BUZZER_PIN); }
      else if(key=='s'||key=='S') {
        mediaPlaying=false; mediaPaused=false; noTone(BUZZER_PIN);
        melodyPos=0;
      }
      else if(key=='n'||key=='N') {
        mediaSelected=(mediaSelected+1)%mediaCount;
        mediaLoadMelody(mediaList[mediaSelected].name);
        melodyPos=0;
      }
      else if(key=='p'||key=='P') {
        mediaSelected=(mediaSelected-1+mediaCount)%mediaCount;
        mediaLoadMelody(mediaList[mediaSelected].name);
        melodyPos=0;
      }
      else if(key==27) { mediaSubMode=MD_LIST; }
      break;

    case MD_HTTP:
      if(key=='p'||key=='P') mediaHTTPControl("play");
      else if(key=='s'||key=='S') mediaHTTPControl("stop");
      else if(key=='n'||key=='N') mediaHTTPControl("next");
      else if(key==27) mediaSubMode=MD_LIST;
      break;
  }
}
