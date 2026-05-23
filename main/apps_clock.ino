// ═══════════════════════════════════════════════════════════
//  apps_clock.ino - Clock display
// ═══════════════════════════════════════════════════════════

void drawClock() {
  oled.clearDisplay();
  if (!rtcReady) {
    oledPrint(0,28,"RTC NOT FOUND");
    oled.display();
    lcdPrint(0,"  RTC ERROR!    ");
    lcdPrint(1,"Check DS3231    ");
    return;
  }
  DateTime now = rtc.now();
  char tb[9];
  snprintf(tb,sizeof(tb),"%02d:%02d:%02d",now.hour(),now.minute(),now.second());
  oled.setTextSize(2); oled.setTextColor(WHITE);
  oled.setCursor(4,8); oled.print(tb); oled.setTextSize(1);
  char db[11];
  snprintf(db,sizeof(db),"%04d-%02d-%02d",now.year(),now.month(),now.day());
  oledPrint(16,34,String(db));
  const char* days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
  oledPrint(0,46,String(days[now.dayOfTheWeek()]));
  char tmp[8];
  snprintf(tmp,sizeof(tmp),"%.1fC",rtc.getTemperature());
  oledPrint(88,46,String(tmp));
  oledPrint(0,56,wifiReady?WiFi.localIP().toString():"No WiFi");
  oled.display();
  char l0[17],l1[17];
  snprintf(l0,sizeof(l0),"Time:%02d:%02d:%02d   ",now.hour(),now.minute(),now.second());
  snprintf(l1,sizeof(l1),"Date:%02d/%02d/%04d",now.day(),now.month(),now.year());
  lcdPrint(0,String(l0)); lcdPrint(1,String(l1));
}
