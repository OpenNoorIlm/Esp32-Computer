//  Library: https://github.com/Caffreyfans/ESP-Arduino-Lua
// ═══════════════════════════════════════════════════════════
// NOTE: LuaWrapper only exposes Lua_register() and Lua_dostring().
// All callbacks use the raw Lua C API (lua_tointeger, lua_tostring, etc.)
// via the lua_State* L parameter passed to each lua_CFunction.
LuaWrapper lua;

// Helper: get lua_State from a global — not needed; each C function receives L directly.
// All functions below are lua_CFunction: int fn(lua_State *L)

// ── Lua API bindings ──────────────────────────────────────

// oled.clear()
int lua_oled_clear(lua_State *L) {
  oled.clearDisplay(); oled.display(); return 0;
}
// oled.print(x, y, text)
int lua_oled_print(lua_State *L) {
  int x    = (int)lua_tointeger(L, 1);
  int y    = (int)lua_tointeger(L, 2);
  String s = String(lua_tostring(L, 3));
  oledPrint(x, y, s); oled.display(); return 0;
}
// oled.pixel(x, y)
int lua_oled_pixel(lua_State *L) {
  oled.drawPixel((int)lua_tointeger(L,1), (int)lua_tointeger(L,2), WHITE);
  oled.display(); return 0;
}
// oled.line(x0,y0,x1,y1)
int lua_oled_line(lua_State *L) {
  oled.drawLine((int)lua_tointeger(L,1), (int)lua_tointeger(L,2),
                (int)lua_tointeger(L,3), (int)lua_tointeger(L,4), WHITE);
  oled.display(); return 0;
}
// oled.rect(x,y,w,h)
int lua_oled_rect(lua_State *L) {
  oled.drawRect((int)lua_tointeger(L,1), (int)lua_tointeger(L,2),
                (int)lua_tointeger(L,3), (int)lua_tointeger(L,4), WHITE);
  oled.display(); return 0;
}
// oled.fill(x,y,w,h)
int lua_oled_fill(lua_State *L) {
  oled.fillRect((int)lua_tointeger(L,1), (int)lua_tointeger(L,2),
                (int)lua_tointeger(L,3), (int)lua_tointeger(L,4), WHITE);
  oled.display(); return 0;
}
// oled.circle(x,y,r)
int lua_oled_circle(lua_State *L) {
  oled.drawCircle((int)lua_tointeger(L,1), (int)lua_tointeger(L,2),
                  (int)lua_tointeger(L,3), WHITE);
  oled.display(); return 0;
}
// oled.invert(bool)
int lua_oled_invert(lua_State *L) {
  oled.invertDisplay((int)lua_tointeger(L,1) != 0); return 0;
}
// lcd.print(row, text)
int lua_lcd_print(lua_State *L) {
  int row  = (int)lua_tointeger(L, 1);
  String s = String(lua_tostring(L, 2));
  lcdPrint(row, s); return 0;
}
// lcd.clear()
int lua_lcd_clear(lua_State *L) { lcdClear(); return 0; }
// rgb.set(r,g,b)
int lua_rgb_set(lua_State *L) {
  rgbSet((int)lua_tointeger(L,1), (int)lua_tointeger(L,2), (int)lua_tointeger(L,3)); return 0;
}
// rgb.off()
int lua_rgb_off(lua_State *L) { rgbOff(); return 0; }
// buzz(freq, ms)
int lua_buzz_fn(lua_State *L) {
  buzz((uint32_t)lua_tointeger(L,1), (uint32_t)lua_tointeger(L,2)); return 0;
}
// key.read() -> string or nil
int lua_key_read(lua_State *L) {
  char k = readKeyboard();
  if (k) { char s[2] = {k, 0}; lua_pushstring(L, s); }
  else   lua_pushnil(L);
  return 1;
}
// key.wait(timeout_ms) -> string or nil
int lua_key_wait(lua_State *L) {
  uint32_t timeout = (uint32_t)lua_tointeger(L, 1);
  if (timeout == 0) timeout = 5000;
  uint32_t start = millis();
  char k = 0;
  while (!k && millis() - start < timeout) { k = readKeyboard(); delay(10); }
  if (k) { char s[2] = {k, 0}; lua_pushstring(L, s); }
  else   lua_pushnil(L);
  return 1;
}
// sd.read(path) -> string or nil
int lua_sd_read(lua_State *L) {
  String path = String(lua_tostring(L, 1));
  File f = SD.open(path.c_str());
  if (!f) { lua_pushnil(L); return 1; }
  String s = f.readString(); f.close();
  lua_pushstring(L, s.c_str()); return 1;
}
// sd.write(path, data) -> bool
int lua_sd_write(lua_State *L) {
  String path = String(lua_tostring(L, 1));
  String data = String(lua_tostring(L, 2));
  File f = SD.open(path.c_str(), FILE_WRITE);
  if (!f) { lua_pushinteger(L, 0); return 1; }
  f.print(data); f.close(); lua_pushinteger(L, 1); return 1;
}
// sd.append(path, data) -> bool
int lua_sd_append(lua_State *L) {
  String path = String(lua_tostring(L, 1));
  String data = String(lua_tostring(L, 2));
  File f = SD.open(path.c_str(), FILE_APPEND);
  if (!f) { lua_pushinteger(L, 0); return 1; }
  f.print(data); f.close(); lua_pushinteger(L, 1); return 1;
}
// sd.exists(path) -> bool
int lua_sd_exists(lua_State *L) {
  String path = String(lua_tostring(L, 1));
  lua_pushinteger(L, SD.exists(path.c_str()) ? 1 : 0); return 1;
}
// sd.remove(path) -> bool
int lua_sd_remove(lua_State *L) {
  String path = String(lua_tostring(L, 1));
  lua_pushinteger(L, SD.remove(path.c_str()) ? 1 : 0); return 1;
}
// sd.mkdir(path) -> bool
int lua_sd_mkdir(lua_State *L) {
  String path = String(lua_tostring(L, 1));
  lua_pushinteger(L, SD.mkdir(path.c_str()) ? 1 : 0); return 1;
}
// http.get(url) -> string or nil
int lua_http_get(lua_State *L) {
  if (!wifiReady) { lua_pushnil(L); return 1; }
  String url = String(lua_tostring(L, 1));
  HTTPClient h; h.begin(url); h.setTimeout(10000);
  int code = h.GET();
  if (code == 200) lua_pushstring(L, h.getString().c_str());
  else             lua_pushnil(L);
  h.end(); return 1;
}
// http.post(url, body) -> string or nil
int lua_http_post(lua_State *L) {
  if (!wifiReady) { lua_pushnil(L); return 1; }
  String url  = String(lua_tostring(L, 1));
  String body = String(lua_tostring(L, 2));
  HTTPClient h; h.begin(url);
  h.addHeader("Content-Type", "application/json");
  h.setTimeout(15000);
  int code = h.POST(body);
  if (code == 200) lua_pushstring(L, h.getString().c_str());
  else             lua_pushnil(L);
  h.end(); return 1;
}
// sys.reboot()
int lua_sys_reboot(lua_State *L) { ESP.restart(); return 0; }
// sys.millis() -> int
int lua_sys_millis(lua_State *L) { lua_pushinteger(L, millis()); return 1; }
// sys.delay(ms)
int lua_sys_delay(lua_State *L) { delay((uint32_t)lua_tointeger(L,1)); return 0; }
// sys.free() -> int
int lua_sys_free(lua_State *L) { lua_pushinteger(L, ESP.getFreeHeap()); return 1; }
// sys.mode(n)
int lua_sys_mode(lua_State *L) {
  currentMode = (AppMode)constrain((int)lua_tointeger(L,1), 0, 10); return 0;
}
// wifi.status() -> 1/0
int lua_wifi_status(lua_State *L) { lua_pushinteger(L, wifiReady ? 1 : 0); return 1; }
// wifi.ip() -> string
int lua_wifi_ip(lua_State *L) {
  String ip = wifiReady ? WiFi.localIP().toString() : "";
  lua_pushstring(L, ip.c_str()); return 1;
}
// rtc.time() -> string
int lua_rtc_time(lua_State *L) {
  if (!rtcReady) { lua_pushnil(L); return 1; }
  DateTime n = rtc.now(); char buf[9];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", n.hour(), n.minute(), n.second());
  lua_pushstring(L, buf); return 1;
}
// rtc.date() -> string
int lua_rtc_date(lua_State *L) {
  if (!rtcReady) { lua_pushnil(L); return 1; }
  DateTime n = rtc.now(); char buf[11];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", n.year(), n.month(), n.day());
  lua_pushstring(L, buf); return 1;
}
// rtc.temp() -> float
int lua_rtc_temp(lua_State *L) {
  lua_pushnumber(L, rtcReady ? rtc.getTemperature() : 0); return 1;
}
// rtc.set(year, month, day, hour, min, sec) -> 1/0
int lua_rtc_set(lua_State *L) {
  if (!rtcReady) { lua_pushinteger(L, 0); return 1; }
  int yr  = (int)lua_tointeger(L, 1);
  int mo  = (int)lua_tointeger(L, 2);
  int dy  = (int)lua_tointeger(L, 3);
  int hr  = (int)lua_tointeger(L, 4);
  int mn  = (int)lua_tointeger(L, 5);
  int sc  = (int)lua_tointeger(L, 6);
  rtc.adjust(DateTime(yr, mo, dy, hr, mn, sc));
  lua_pushinteger(L, 1); return 1;
}

// ── Register all libs ─────────────────────────────────────
void luaRegisterLibs() {
  // oled
  lua.Lua_register("oled_clear",   lua_oled_clear);
  lua.Lua_register("oled_print",   lua_oled_print);
  lua.Lua_register("oled_pixel",   lua_oled_pixel);
  lua.Lua_register("oled_line",    lua_oled_line);
  lua.Lua_register("oled_rect",    lua_oled_rect);
  lua.Lua_register("oled_fill",    lua_oled_fill);
  lua.Lua_register("oled_circle",  lua_oled_circle);
  lua.Lua_register("oled_invert",  lua_oled_invert);
  // lcd
  lua.Lua_register("lcd_print",    lua_lcd_print);
  lua.Lua_register("lcd_clear",    lua_lcd_clear);
  // rgb
  lua.Lua_register("rgb_set",      lua_rgb_set);
  lua.Lua_register("rgb_off",      lua_rgb_off);
  // key
  lua.Lua_register("key_read",     lua_key_read);
  lua.Lua_register("key_wait",     lua_key_wait);
  // sd
  lua.Lua_register("sd_read",      lua_sd_read);
  lua.Lua_register("sd_write",     lua_sd_write);
  lua.Lua_register("sd_append",    lua_sd_append);
  lua.Lua_register("sd_exists",    lua_sd_exists);
  lua.Lua_register("sd_remove",    lua_sd_remove);
  lua.Lua_register("sd_mkdir",     lua_sd_mkdir);
  // http
  lua.Lua_register("http_get",     lua_http_get);
  lua.Lua_register("http_post",    lua_http_post);
  // sys
  lua.Lua_register("sys_reboot",   lua_sys_reboot);
  lua.Lua_register("sys_millis",   lua_sys_millis);
  lua.Lua_register("sys_delay",    lua_sys_delay);
  lua.Lua_register("sys_free",     lua_sys_free);
  lua.Lua_register("sys_mode",     lua_sys_mode);
  // wifi
  lua.Lua_register("wifi_status",  lua_wifi_status);
  lua.Lua_register("wifi_ip",      lua_wifi_ip);
  // rtc
  lua.Lua_register("rtc_time",     lua_rtc_time);
  lua.Lua_register("rtc_date",     lua_rtc_date);
  lua.Lua_register("rtc_temp",     lua_rtc_temp);
  lua.Lua_register("rtc_set",      lua_rtc_set);
  // buzz
  lua.Lua_register("buzz",         lua_buzz_fn);

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
rtc={time=rtc_time, date=rtc_date, temp=rtc_temp, set=rtc_set}
)";
  lua.Lua_dostring(&init);
}

void initLua() {
  luaRegisterLibs();
}

void runLuaString(const String& code) {
  String s = code;
  String err = lua.Lua_dostring(&s);
  if (err.length() > 0) {
    oledHeader("LUA ERROR");
    oledPrint(0, 14, err.substring(0,  16));
    oledPrint(0, 26, err.substring(16, 32));
    oledPrint(0, 38, err.substring(32, 48));
    oled.display();
    errorBeep();
    delay(3000);
  }
}


