---
name: flashing-toolchain
description: How to compile/flash this ESP32-S3 sketch (arduino-cli path, FQBN, port)
metadata:
  type: reference
---

Compile & flash `smart-eink-display.ino` to the user's ESP32-S3 with the arduino-cli bundled inside Arduino IDE 2.x:

- CLI: `C:/Program Files/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe`
- FQBN: `esp32:esp32:esp32s3`
- Board is on **COM3** (USB serial). Serial monitor at 115200 baud.
- Compile: `arduino-cli compile --fqbn esp32:esp32:esp32s3 <sketchdir>`
- Flash: `arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32s3 <sketchdir>`

Required libs (all now installed): GxEPD2, Adafruit GFX, Adafruit BusIO, ArduinoJson (v7). ArduinoJson was the one missing dependency.

Sketch uses ~85% of the default 1.3MB app partition — near-full but fits. If it grows, switch to a larger partition scheme (`--build-property`/board option).
