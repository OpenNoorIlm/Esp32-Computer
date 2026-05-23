-- Interactive counter app
local count = 0
local running = true
while running do
  oled.clear()
  oled.print(40, 10, "COUNTER")
  oled.print(52, 28, tostring(count))
  oled.print(0, 50, "+/-=count Q=quit")
  lcd.print(0, "Counter App")
  lcd.print(1, "Count: " .. count)
  local k = key.wait(10000)
  if k == "+" or k == "d" then count = count + 1
  elseif k == "-" or k == "a" then count = count - 1
  elseif k == "r" then count = 0
  elseif k == "q" or k == "Q" then running = false end
end
oled.clear()
lcd.clear()
