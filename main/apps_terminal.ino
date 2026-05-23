// ═══════════════════════════════════════════════════════════
//  apps_terminal.ino
// ═══════════════════════════════════════════════════════════

String termLines[4];
String termInput = "";

void termAdd(const String& line) {
  for (int i=0;i<3;i++) termLines[i]=termLines[i+1];
  termLines[3]=line;
}

void termProcess(const String& cmd) {
  String c=cmd; c.trim();
  if (c=="help") {
    termAdd("time date temp");
    termAdd("ls mkdir rm mv");
    termAdd("wifi ip reboot");
    termAdd("rgb about clear");
  } else if (c=="time") {
    if(rtcReady){DateTime n=rtc.now();char b[9];
      snprintf(b,sizeof(b),"%02d:%02d:%02d",n.hour(),n.minute(),n.second());
      termAdd(String(b));} else termAdd("RTC not found!");
  } else if (c=="date") {
    if(rtcReady){DateTime n=rtc.now();char b[11];
      snprintf(b,sizeof(b),"%04d-%02d-%02d",n.year(),n.month(),n.day());
      termAdd(String(b));} else termAdd("RTC not found!");
  } else if (c=="temp") {
    if(rtcReady) termAdd("T:"+String(rtc.getTemperature(),1)+"C");
    else termAdd("RTC not found!");
  } else if (c=="wifi") {
    termAdd(wifiReady?"WiFi:"+WiFi.localIP().toString():"Not connected");
  } else if (c=="ip") {
    termAdd(wifiReady?WiFi.localIP().toString():"No WiFi");
  } else if (c=="clear") {
    for(int i=0;i<4;i++) termLines[i]="";
  } else if (c=="about") {
    termAdd("ESP32 MiniPC v3");
    termAdd("10 Apps + Lua");
  } else if (c=="reboot") {
    termAdd("Rebooting..."); delay(500); ESP.restart();
  } else if (c.startsWith("rgb ")) {
    int r,g,b;
    if(sscanf(c.substring(4).c_str(),"%d %d %d",&r,&g,&b)==3){
      rgbSet(r,g,b); termAdd("RGB set!");
    } else termAdd("rgb R G B");
  } else if (c.startsWith("ls")) {
    String path = c.length()>3 ? c.substring(3) : "/";
    if(sdReady){
      File dir=SD.open(path.c_str()); int cnt=0;
      while(cnt<5){File e=dir.openNextFile();if(!e)break;
        termAdd((e.isDirectory()?"[D]":"[F]")+String(e.name()));
        e.close();cnt++;}
      dir.close();
    } else termAdd("SD not ready!");
  } else if (c.startsWith("mkdir ")) {
    String path=c.substring(6);
    if(sdReady){SD.mkdir(path.c_str());termAdd("Created:"+path);}
    else termAdd("SD not ready!");
  } else if (c.startsWith("rm ")) {
    String path=c.substring(3);
    if(sdReady){SD.remove(path.c_str());termAdd("Removed:"+path);}
    else termAdd("SD not ready!");
  } else if (c.startsWith("cat ")) {
    String path=c.substring(4);
    if(sdReady){File f=SD.open(path.c_str());
      if(f){String line=f.readStringUntil('\n');termAdd(line.substring(0,14));f.close();}
      else termAdd("Not found!");}
    else termAdd("SD not ready!");
  } else if (c.length()>0) {
    termAdd("Unknown: "+c);
  }
}

void drawTerminal() {
  oledHeader("> TERMINAL");
  for (int i=0;i<4;i++) oledPrint(0,12+i*12,termLines[i]);
  oled.drawFastHLine(0,54,OLED_W,WHITE);
  oledPrint(0,56,"$"+termInput+"_");
  oled.display();
  lcdPrint(0,"TERMINAL        ");
  lcdPrint(1,termLines[3].length()>0?termLines[3]:"type 'help'     ");
}
