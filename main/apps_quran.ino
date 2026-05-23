// ═══════════════════════════════════════════════════════════
//  apps_quran.ino
// ═══════════════════════════════════════════════════════════

int    quranSurah  = 1;
int    quranAyah   = 1;
int    quranScroll = 0;
bool   quranLoaded = false;
int    quranSubMode = 0; // 0=surah list 1=ayah view
String quranText[4]; // ar,en,kz,jl
int    quranTextOffset = 0; // horizontal scroll for long text

const char* quranSurahNames[] = {
  "Al-Fatihah","Al-Baqarah","Ali Imran","An-Nisa","Al-Maidah",
  "Al-Anam","Al-Araf","Al-Anfal","At-Tawbah","Yunus",
  "Hud","Yusuf","Ar-Rad","Ibrahim","Al-Hijr",
  "An-Nahl","Al-Isra","Al-Kahf","Maryam","Ta-Ha",
  "Al-Anbya","Al-Hajj","Al-Muminun","An-Nur","Al-Furqan",
  "Ash-Shuara","An-Naml","Al-Qasas","Al-Ankabut","Ar-Rum",
  "Luqman","As-Sajdah","Al-Ahzab","Saba","Fatir",
  "Ya-Sin","As-Saffat","Sad","Az-Zumar","Ghafir",
  "Fussilat","Ash-Shura","Az-Zukhruf","Ad-Dukhan","Al-Jathiyah",
  "Al-Ahqaf","Muhammad","Al-Fath","Al-Hujurat","Qaf",
  "Adh-Dhariyat","At-Tur","An-Najm","Al-Qamar","Ar-Rahman",
  "Al-Waqiah","Al-Hadid","Al-Mujadila","Al-Hashr","Al-Mumtahanah",
  "As-Saf","Al-Jumuah","Al-Munafiqun","At-Taghabun","At-Talaq",
  "At-Tahrim","Al-Mulk","Al-Qalam","Al-Haqqah","Al-Maarij",
  "Nuh","Al-Jinn","Al-Muzzammil","Al-Muddaththir","Al-Qiyamah",
  "Al-Insan","Al-Mursalat","An-Naba","An-Naziat","Abasa",
  "At-Takwir","Al-Infitar","Al-Mutaffifin","Al-Inshiqaq","Al-Buruj",
  "At-Tariq","Al-Ala","Al-Ghashiyah","Al-Fajr","Al-Balad",
  "Ash-Shams","Al-Layl","Ad-Duha","Ash-Sharh","At-Tin",
  "Al-Alaq","Al-Qadr","Al-Bayyinah","Az-Zalzalah","Al-Adiyat",
  "Al-Qariah","At-Takathur","Al-Asr","Al-Humazah","Al-Fil",
  "Quraysh","Al-Maun","Al-Kawthar","Al-Kafirun","An-Nasr",
  "Al-Masad","Al-Ikhlas","Al-Falaq","An-Nas"
};

void quranLoadAyah() {
  if (!sdReady) return;
  File f = SD.open("/quran.json");
  if (!f) return;
  quranText[0]=quranText[1]=quranText[2]=quranText[3]="";
  DynamicJsonDocument doc(32768);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) { quranText[0]="JSON Error!"; quranLoaded=true; return; }
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject s : arr) {
    if (s["s"].as<int>() == quranSurah) {
      JsonArray ayahs = s["ayahs"].as<JsonArray>();
      for (JsonObject a : ayahs) {
        if (a["n"].as<int>() == quranAyah) {
          quranText[0] = a["ar"].as<String>();
          quranText[1] = a["en"].as<String>();
          quranText[2] = a["kz"].as<String>();
          quranText[3] = a["jl"].as<String>();
          quranLoaded  = true;
          quranTextOffset = 0;
          return;
        }
      }
    }
  }
  quranText[1]="Ayah not found";
  quranLoaded=true;
}

void drawQuranSurahList() {
  oledHeader("QURAN - SURAHS");
  int start = ((quranSurah-1)/4)*4;
  for (int i=0; i<4 && (start+i)<114; i++) {
    int idx = start+i;
    bool sel = (idx+1)==quranSurah;
    String line = (sel?">":"") + String(idx+1)+"."+String(quranSurahNames[idx]);
    oledPrint(0, 12+i*12, line.substring(0,16), WHITE);
  }
  oledPrint(0,56,"ENTER=open W/S=nav");
  oled.display();
  lcdPrint(0,"QURAN SURAHS    ");
  lcdPrint(1,String(quranSurah)+"."+String(quranSurahNames[quranSurah-1]));
}

void drawQuranAyah() {
  oledHeader("S"+String(quranSurah)+":A"+String(quranAyah));
  if (!quranLoaded) {
    oledPrint(0,28,"Loading...");
    oled.display();
    quranLoadAyah();
  }
  String labels[]={"AR","EN","KZ","JL"};
  int showIdx = (cfg.quranTrans==3) ? quranScroll : cfg.quranTrans;
  String fullText = "["+labels[showIdx]+"] "+quranText[showIdx];
  // Show in 3 lines with offset
  int charsPerLine = 16;
  int offset = quranTextOffset * charsPerLine;
  int y = 13;
  for (int row=0; row<3 && y<54; row++) {
    int start = offset + row*charsPerLine;
    if (start >= (int)fullText.length()) break;
    oledPrint(0, y, fullText.substring(start, start+charsPerLine));
    y+=13;
  }
  oled.drawFastHLine(0,54,OLED_W,WHITE);
  String hint = "W/S:ayah A/D:sur Q:trans";
  oledPrint(0,56,hint);
  oled.display();
  lcdPrint(0,"S"+String(quranSurah)+" A"+String(quranAyah)+"        ");
  lcdPrint(1,wrapLine(quranText[1],16));
}

void drawQuran() {
  if (quranSubMode==0) drawQuranSurahList();
  else drawQuranAyah();
}

void handleQuranKey(char key) {
  if (quranSubMode==0) {
    if (key=='w'||key=='W') { if(quranSurah>1) quranSurah--; }
    else if (key=='s'||key=='S') { if(quranSurah<114) quranSurah++; }
    else if (key=='\n'||key=='\r') { quranSubMode=1; quranAyah=1; quranLoaded=false; }
    else if (key==27) { currentMode=MODE_CLOCK; }
  } else {
    if (key=='w'||key=='W') { if(quranAyah>1){quranAyah--;quranLoaded=false;} }
    else if(key=='s'||key=='S') { quranAyah++; quranLoaded=false; }
    else if(key=='a'||key=='A') { if(quranSurah>1){quranSurah--;quranAyah=1;quranLoaded=false;} }
    else if(key=='d'||key=='D') { if(quranSurah<114){quranSurah++;quranAyah=1;quranLoaded=false;} }
    else if(key=='q'||key=='Q') { quranScroll=(quranScroll+1)%4; }
    else if(key=='+') { quranTextOffset++; }
    else if(key=='-') { if(quranTextOffset>0) quranTextOffset--; }
    else if(key==27) { quranSubMode=0; }
  }
}
