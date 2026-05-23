// ═══════════════════════════════════════════════════════════
//  apps_browser.ino - Web browser (text mode)
// ═══════════════════════════════════════════════════════════

String browserURL     = "http://";
String browserContent = "";
int    browserOffset  = 0;
int    browserChunkSize = 16;
bool   browserLoading = false;
enum BrowserSubMode { BR_INPUT, BR_VIEW };
BrowserSubMode browserSubMode = BR_INPUT;

void browserFetch(const String& url) {
  if (!wifiReady) { browserContent="No WiFi!"; return; }
  browserLoading=true;
  browserContent="Loading...";
  lcdPrint(0,"Fetching...     ");
  lcdPrint(1,url.substring(0,16));

  HTTPClient http;
  http.begin(url);
  http.setTimeout(15000);
  http.addHeader("User-Agent","ESP32Browser/1.0");
  int code=http.GET();
  if(code==200) {
    String raw=http.getString();
    browserContent=stripHTML(raw);
    // Trim to 4KB to save RAM
    if(browserContent.length()>4096) {
      browserContent=browserContent.substring(0,4096)+"...[truncated]";
    }
    browserOffset=0;
    okBeep();
  } else {
    browserContent="HTTP Error: "+String(code);
    errorBeep();
  }
  http.end();
  browserLoading=false;
}

void drawBrowserInput() {
  oledHeader("> WEB BROWSER");
  oledPrint(0,14,"URL:");
  oledPrint(0,26,browserURL.substring(max(0,(int)browserURL.length()-16)));
  oledPrint(0,40,"_");
  oledPrint(0,54,"ENTER=go ESC=back");
  oled.display();
  lcdPrint(0,"WEB BROWSER     ");
  lcdPrint(1,browserURL.substring(0,16));
}

void drawBrowserView() {
  oledHeader(browserLoading?"Loading...":"BROWSER");
  int charsPerLine=16;
  int linesPerPage=4;
  int offset=browserOffset*charsPerLine;
  for(int row=0;row<linesPerPage;row++) {
    int start=offset+row*charsPerLine;
    if(start>=(int)browserContent.length()) break;
    oledPrint(0,12+row*12,browserContent.substring(start,start+charsPerLine));
  }
  oledPrint(0,56,"W/S:scroll U=url ESC=bk");
  oled.display();
  lcdPrint(0,"BROWSER VIEW    ");
  int totalLines=browserContent.length()/charsPerLine;
  lcdPrint(1,"Line "+String(browserOffset)+"/"+String(totalLines)+"      ");
}

void drawBrowser() {
  if(browserSubMode==BR_INPUT) drawBrowserInput();
  else drawBrowserView();
}

void handleBrowserKey(char key) {
  if(browserSubMode==BR_INPUT) {
    if(key=='\n'||key=='\r') {
      if(browserURL.length()>7) {
        browserFetch(browserURL);
        browserSubMode=BR_VIEW;
      }
    } else if(key==27) { currentMode=MODE_CLOCK; }
    else if((key==8||key==127)&&browserURL.length()>0)
      browserURL.remove(browserURL.length()-1);
    else if(browserURL.length()<80) browserURL+=key;
  } else {
    if(key=='w'||key=='W') { if(browserOffset>0) browserOffset--; }
    else if(key=='s'||key=='S') browserOffset++;
    else if(key=='u'||key=='U') browserSubMode=BR_INPUT;
    else if(key==27) browserSubMode=BR_INPUT;
  }
}
