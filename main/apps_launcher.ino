// ═══════════════════════════════════════════════════════════
//  apps_launcher.ino - Lua app launcher (LuaWrapper version)
// ═══════════════════════════════════════════════════════════

String  luaAppList[16];
int     luaAppCount    = 0;
int     luaAppSelected = 0;

void loadAppList() {
  luaAppCount = 0; luaAppSelected = 0;
  if (!sdReady) return;
  File dir = SD.open(cfg.appsPath.c_str());
  if (!dir) { SD.mkdir(cfg.appsPath.c_str()); return; }
  while (luaAppCount < 16) {
    File f = dir.openNextFile(); if (!f) break;
    String name = String(f.name());
    if (name.endsWith(".lua")) luaAppList[luaAppCount++] = name;
    f.close();
  }
  dir.close();
}

void runLuaApp(const String& filename) {
  if (!sdReady) return;
  String path = cfg.appsPath + filename;
  File f = SD.open(path.c_str());
  if (!f) {
    oledHeader("ERROR");
    oledPrint(0, 28, "App not found!");
    oled.display(); delay(1500); return;
  }
  String code = f.readString(); f.close();
  appRunning = true;
  rgbWhite();
  lcdPrint(0, "Running Lua App ");
  lcdPrint(1, filename.substring(0, 16));
#ifdef LUAWRAPPER_H
  runLuaString(code);
#else
  oledHeader("ERROR");
  oledPrint(0, 28, "Lua not built!");
  oled.display(); delay(1500);
#endif
  appRunning = false;
  rgbGreen();
}

void drawLauncher() {
  oledHeader("> APP LAUNCHER");
  if (!sdReady) {
    oledPrint(0, 28, "SD not ready!"); oled.display();
    lcdPrint(0, "LAUNCHER        "); lcdPrint(1, "SD Not Found!   "); return;
  }
  if (luaAppCount == 0) {
    oledPrint(0, 16, "No Lua apps found");
    oledPrint(0, 28, cfg.appsPath);
    oledPrint(0, 40, "Add .lua files!");
    oledPrint(0, 52, "R=refresh");
    oled.display();
    lcdPrint(0, "LAUNCHER        "); lcdPrint(1, "No .lua apps    "); return;
  }
  int start = (luaAppSelected / 4) * 4;
  for (int i = 0; i < 4 && (start+i) < luaAppCount; i++) {
    int idx = start + i;
    String prefix = (idx == luaAppSelected) ? ">" : " ";
    String name = luaAppList[idx];
    if (name.endsWith(".lua")) name = name.substring(0, name.length()-4);
    oledPrint(0, 12+i*12, prefix+name.substring(0, 15));
  }
  oledPrint(0, 56, "ENTER=run R=refresh");
  oled.display();
  lcdPrint(0, "APP LAUNCHER    ");
  String n = luaAppList[luaAppSelected];
  if (n.endsWith(".lua")) n = n.substring(0, n.length()-4);
  lcdPrint(1, n.substring(0, 16));
}

void handleLauncherKey(char key) {
  if (key=='w'||key=='W') { if(luaAppSelected>0) luaAppSelected--; }
  else if (key=='s'||key=='S') { if(luaAppSelected<luaAppCount-1) luaAppSelected++; }
  else if (key=='\n'||key=='\r') { if(luaAppCount>0) runLuaApp(luaAppList[luaAppSelected]); }
  else if (key=='r'||key=='R') { loadAppList(); }
  else if (key==27) currentMode=MODE_CLOCK;
}
