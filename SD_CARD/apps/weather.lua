-- Weather via HTTP
oled.clear()
oled.print(0, 0, "Fetching weather")
local ip = wifi.ip()
if ip == "" then
  oled.print(0, 20, "No WiFi!")
  sys.delay(2000)
  return
end
local res = http.get("http://wttr.in/?format=3")
if res then
  oled.clear()
  oled.print(0, 0, "Weather:")
  oled.print(0, 16, res:sub(1,16))
  oled.print(0, 28, res:sub(17,32))
  lcd.print(0, "Weather:")
  lcd.print(1, res:sub(1,16))
else
  oled.print(0, 20, "Fetch failed!")
end
sys.delay(5000)
