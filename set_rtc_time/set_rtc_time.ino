// ═══════════════════════════════════════════════════════════
//  set_rtc_time.ino
//  Upload this once to set your DS3231 RTC, then re-upload
//  your main firmware. SDA=21, SCL=22 (ESP32 default).
// ═══════════════════════════════════════════════════════════

#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

// ── SET YOUR DATE & TIME HERE ────────────────────────────
#define SET_YEAR    2026
#define SET_MONTH      5
#define SET_DAY       23
#define SET_HOUR      16   // 24-hour
#define SET_MINUTE    36
#define SET_SECOND     0
// ─────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(21, 22);   // SDA, SCL — change if your board differs

  Serial.println("\n=== DS3231 RTC Setter ===");

  if (!rtc.begin()) {
    Serial.println("ERROR: DS3231 not found! Check wiring.");
    Serial.println("SDA=21  SCL=22  VCC=3.3V  GND=GND");
    while (1) delay(1000);
  }

  // Set the time
  rtc.adjust(DateTime(SET_YEAR, SET_MONTH, SET_DAY,
                      SET_HOUR, SET_MINUTE, SET_SECOND));

  // Read back and confirm
  DateTime now = rtc.now();
  char buf[32];
  snprintf(buf, sizeof(buf), "Set to: %04d-%02d-%02d  %02d:%02d:%02d",
           now.year(), now.month(),  now.day(),
           now.hour(), now.minute(), now.second());
  Serial.println(buf);
  Serial.println("Done! Now upload your main firmware.");

  // Blink built-in LED to signal success
  pinMode(2, OUTPUT);
  for (int i = 0; i < 6; i++) {
    digitalWrite(2, !digitalRead(2));
    delay(250);
  }
}

void loop() {
  // Print time every second so you can verify it's running
  DateTime now = rtc.now();
  char buf[24];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d  %02d:%02d:%02d",
           now.year(), now.month(),  now.day(),
           now.hour(), now.minute(), now.second());
  Serial.println(buf);

  if (rtc.lostPower()) {
    Serial.println("WARNING: RTC lost power — battery may be dead.");
  }

  delay(1000);
}
