-- Quick notes saved to SD
local note = ""
local running = true
-- Load existing note
local existing = sd.read("/notes.txt")
if existing then note = existing end
oled.clear()
oled.print(0, 0, "NOTES (ENTER=save)")
oled.print(0, 16, note:sub(1, 16))
oled.print(0, 28, note:sub(17, 32))
while running do
  local k = key.wait(30000)
  if not k then break end
  if k == "\r" or k == "\n" then
    sd.write("/notes.txt", note)
    oled.print(0, 48, "Saved!")
    sys.delay(1000)
    running = false
  elseif k:byte() == 8 or k:byte() == 127 then
    if #note > 0 then note = note:sub(1, #note-1) end
  elseif k == "q" then running = false
  else note = note .. k end
  oled.clear()
  oled.print(0, 0, "NOTES")
  oled.print(0, 16, note:sub(1, 16))
  oled.print(0, 28, note:sub(17, 32))
end
