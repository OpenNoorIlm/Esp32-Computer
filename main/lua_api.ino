//  Library: https://github.com/Caffreyfans/ESP-Arduino-Lua
// ═══════════════════════════════════════════════════════════
// NOTE: LuaWrapper only exposes Lua_register() and Lua_dostring().
// All callbacks use the raw Lua C API (lua_tointeger, lua_tostring, etc.)
// via the lua_State* L parameter passed to each lua_CFunction.

#include <LuaWrapper.h>
LuaWrapper lua;

// Helper: get lua_State from a global — not needed; each C function receives L directly.
// All functions below are lua_CFunction: int fn(lua_State *L)

// ── Lua API bindings ──────────────────────────────────────

// oled.clear()
int lua_oled_clear() {
  oled.clearDisplay(); oled.display(); return 0;
}
// oled.print(x, y, text)
int lua_oled_print() {
  int x    = lua.getinteger(1);
  int y    = lua.getinteger(2);
  String s = lua.getstring(3);
  oledPrint(x, y, s); oled.display(); return 0;
}
// oled.pixel(x, y)
int lua_oled_pixel() {
  oled.drawPixel(lua.getinteger(1), lua.getinteger(2), WHITE);
  oled.display(); return 0;
}
// oled.line(x0,y0,x1,y1)
int lua_oled_line() {
  oled.drawLine(lua.getinteger(1), lua.getinteger(2),
                lua.getinteger(3), lua.getinteger(4), WHITE);
  oled.display(); return 0;
}
// oled.rect(x,y,w,h)
int lua_oled_rect() {
  oled.drawRect(lua.getinteger(1), lua.getinteger(2),
                lua.getinteger(3), lua.getinteger(4), WHITE);
  oled.display(); return 0;
}
// oled.fill(x,y,w,h)
int lua_oled_fill() {
  oled.fillRect(lua.getinteger(1), lua.getinteger(2),
                lua.getinteger(3), lua.getinteger(4), WHITE);
  oled.display(); return 0;
}
// oled.circle(x,y,r)
int lua_oled_circle() {
  oled.drawCircle(lua.getinteger(1), lua.getinteger(2),
                  lua.getinteger(3), WHITE);
  oled.display(); return 0;
}
// oled.invert(bool)
int lua_oled_invert() {
  oled.invertDisplay(lua.getinteger(1) != 0); return 0;
}
// lcd.print(row, text)
int lua_lcd_print() {
  int row  = lua.getinteger(1);
  String s = lua.getstring(2);
  lcdPrint(row, s); return 0;
}
// lcd.clear()
int lua_lcd_clear() { lcdClear(); return 0; }
// rgb.set(r,g,b)
int lua_rgb_set() {
  rgbSet(lua.getinteger(1), lua.getinteger(2), lua.getinteger(3)); return 0;
}
// rgb.off()
int lua_rgb_off() { rgbOff(); return 0; }
// buzz(freq, ms)
int lua_buzz_fn() {
  buzz(lua.getinteger(1), lua.getinteger(2)); return 0;
}
// key.read() -> string or nil
int lua_key_read() {
  char k = readKeyboard();
  if (k) lua.pushstring(String(k));
  else   lua.pushnil();
  return 1;
}
// key.wait(timeout_ms) -> string or nil
int lua_key_wait() {
  uint32_t timeout = lua.getinteger(1);
  if (timeout == 0) timeout = 5000;
  uint32_t start = millis();
  char k = 0;
  while (!k && millis() - start < timeout) { k = readKeyboard(); delay(10); }
  if (k) lua.pushstring(String(k));
  else   lua.pushnil();
  return 1;
}
// sd.read(path) -> string or nil
int lua_sd_read() {
  String path = lua.getstring(1);
  File f = SD.open(path.c_str());
  if (!f) { lua.pushnil(); return 1; }
  String s = f.readString(); f.close();
  lua.pushstring(s); return 1;
}
// sd.write(path, data) -> bool
int lua_sd_write() {
  String path = lua.getstring(1);
  String data = lua.getstring(2);
  File f = SD.open(path.c_str(), FILE_WRITE);
  if (!f) { lua.pushinteger(0); return 1; }
  f.print(data); f.close(); lua.pushinteger(1); return 1;
}
// sd.append(path, data) -> bool
int lua_sd_append() {
  String path = lua.getstring(1);
  String data = lua.getstring(2);
  File f = SD.open(path.c_str(), FILE_APPEND);
  if (!f) { lua.pushinteger(0); return 1; }
  f.print(data); f.close(); lua.pushinteger(1); return 1;
}
// sd.exists(path) -> bool
int lua_sd_exists() {
  String path = lua.getstring(1);
  lua.pushinteger(SD.exists(path.c_str()) ? 1 : 0); return 1;
}
// sd.remove(path) -> bool
int lua_sd_remove() {
  String path = lua.getstring(1);
  lua.pushinteger(SD.remove(path.c_str()) ? 1 : 0); return 1;
}
// sd.mkdir(path) -> bool
int lua_sd_mkdir() {
  String path = lua.getstring(1);
  lua.pushinteger(SD.mkdir(path.c_str()) ? 1 : 0); return 1;
}
// http.get(url) -> string or nil
int lua_http_get() {
  if (!wifiReady) { lua.pushnil(); return 1; }
  String url = lua.getstring(1);
  HTTPClient h; h.begin(url); h.setTimeout(10000);
  int code = h.GET();
  if (code == 200) lua.pushstring(h.getString());
  else             lua.pushnil();
  h.end(); return 1;
}
// http.post(url, body) -> string or nil
int lua_http_post() {
  if (!wifiReady) { lua.pushnil(); return 1; }
  String url  = lua.getstring(1);
  String body = lua.getstring(2);
  HTTPClient h; h.begin(url);
  h.addHeader("Content-Type", "application/json");
  h.setTimeout(15000);
  int code = h.POST(body);
  if (code == 200) lua.pushstring(h.getString());
  else             lua.pushnil();
  h.end(); return 1;
}
// sys.reboot()
int lua_sys_reboot() { ESP.restart(); return 0; }
// sys.millis() -> int
int lua_sys_millis() { lua.pushinteger(millis()); return 1; }
// sys.delay(ms)
int lua_sys_delay() { delay(lua.getinteger(1)); return 0; }
// sys.free() -> int
int lua_sys_free() { lua.pushinteger(ESP.getFreeHeap()); return 1; }
// sys.mode(n)
int lua_sys_mode() {
  currentMode = (AppMode)constrain(lua.getinteger(1), 0, 10); return 0;
}
// wifi.status() -> 1/0
int lua_wifi_status() { lua.pushinteger(wifiReady ? 1 : 0); return 1; }
// wifi.ip() -> string
int lua_wifi_ip() {
  lua.pushstring(wifiReady ? WiFi.localIP().toString() : ""); return 1;
}
// rtc.time() -> string
int lua_rtc_time() {
  if (!rtcReady) { lua.pushnil(); return 1; }
  DateTime n = rtc.now(); char buf[9];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", n.hour(), n.minute(), n.second());
  lua.pushstring(String(buf)); return 1;
}
// rtc.date() -> string
int lua_rtc_date() {
  if (!rtcReady) { lua.pushnil(); return 1; }
  DateTime n = rtc.now(); char buf[11];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", n.year(), n.month(), n.day());
  lua.pushstring(String(buf)); return 1;
}
// rtc.temp() -> float
int lua_rtc_temp() {
  lua.pushnumber(rtcReady ? rtc.getTemperature() : 0); return 1;
}

// ── Register all libs ─────────────────────────────────────
void luaRegisterLibs() {
  // oled
  lua.registerfunction("oled_clear",   lua_oled_clear);
  lua.registerfunction("oled_print",   lua_oled_print);
  lua.registerfunction("oled_pixel",   lua_oled_pixel);
  lua.registerfunction("oled_line",    lua_oled_line);
  lua.registerfunction("oled_rect",    lua_oled_rect);
  lua.registerfunction("oled_fill",    lua_oled_fill);
  lua.registerfunction("oled_circle",  lua_oled_circle);
  lua.registerfunction("oled_invert",  lua_oled_invert);
  // lcd
  lua.registerfunction("lcd_print",    lua_lcd_print);
  lua.registerfunction("lcd_clear",    lua_lcd_clear);
  // rgb
  lua.registerfunction("rgb_set",      lua_rgb_set);
  lua.registerfunction("rgb_off",      lua_rgb_off);
  // key
  lua.registerfunction("key_read",     lua_key_read);
  lua.registerfunction("key_wait",     lua_key_wait);
  // sd
  lua.registerfunction("sd_read",      lua_sd_read);
  lua.registerfunction("sd_write",     lua_sd_write);
  lua.registerfunction("sd_append",    lua_sd_append);
  lua.registerfunction("sd_exists",    lua_sd_exists);
  lua.registerfunction("sd_remove",    lua_sd_remove);
  lua.registerfunction("sd_mkdir",     lua_sd_mkdir);
  // http
  lua.registerfunction("http_get",     lua_http_get);
  lua.registerfunction("http_post",    lua_http_post);
  // sys
  lua.registerfunction("sys_reboot",   lua_sys_reboot);
  lua.registerfunction("sys_millis",   lua_sys_millis);
  lua.registerfunction("sys_delay",    lua_sys_delay);
  lua.registerfunction("sys_free",     lua_sys_free);
  lua.registerfunction("sys_mode",     lua_sys_mode);
  // wifi
  lua.registerfunction("wifi_status",  lua_wifi_status);
  lua.registerfunction("wifi_ip",      lua_wifi_ip);
  // rtc
  lua.registerfunction("rtc_time",     lua_rtc_time);
  lua.registerfunction("rtc_date",     lua_rtc_date);
  lua.registerfunction("rtc_temp",     lua_rtc_temp);
  // buzz
  lua.registerfunction("buzz",         lua_buzz_fn);

  // Create Lua table aliases so scripts use oled.print() syntax
  String init = R"(
oled={
  clear=oled_clear, print=oled_print, pixel=oled_pixel,
  line=oled_line,   rect=oled_rect,   fill=oled_fill,
  circle=oled_circle, invert=oled_invert
}
lcd={print=lcd_print, clear=lcd_clear}
rgb={set=rgb_set, off=rgb_off}
key={read=key_read, wait=key_wait}
sd={read=sd_read, write=sd_write, append=sd_append,
    exists=sd_exists, remove=sd_remove, mkdir=sd_mkdir}
http={get=http_get, post=http_post}
sys={reboot=sys_reboot, millis=sys_millis, delay=sys_delay,
     free=sys_free, mode=sys_mode}
wifi={status=wifi_status, ip=wifi_ip}
rtc={time=rtc_time, date=rtc_date, temp=rtc_temp}
)";
  lua.Lua_dostring(&init);
}

void initLua() {
  luaRegisterLibs();
}

void runLuaString(const String& code) {
  String s = code;
  String err = lua.Lua_dostring(&s);
  if (err.length() > 0 && err != "0") {
    oledHeader("LUA ERROR");
    oledPrint(0, 14, err.substring(0,  16));
    oledPrint(0, 26, err.substring(16, 32));
    oledPrint(0, 38, err.substring(32, 48));
    oled.display();
    errorBeep();
    delay(3000);
  }
}


