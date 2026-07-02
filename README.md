# smart-eink-display — ESP32-S3 networked e-paper display

A single, flashable Arduino sketch that turns an **ESP32-S3 + 7.5" 800×480
black/white e-paper panel** (GoodDisplay / Waveshare **GDEY075T7**) into a
network display you can push text and images to over a small HTTP JSON API.

On boot the panel shows its own health and status; after that it serves a REST
API and a tiny web dashboard.

![boot screen: name, IP, host, WiFi, resolution, memory, firmware]

## Features

- **Self-describing boot screen** — device name, IP address, `*.local`
  hostname, WiFi SSID + signal strength, resolution **and aspect ratio**,
  firmware version, and free memory.
- **Lightweight HTTP JSON API** — push text (with word-wrap), clear the screen,
  upload a 1-bit BMP image, or read live status.
- **Built-in web dashboard** at `/` for quick manual control.
- **Graceful networking** — if WiFi fails to connect it falls back to an open
  SoftAP so the device is still reachable; mDNS advertises `eink-display.local`.
- **Unit-tested core logic** — the pure rendering/parsing helpers are covered by
  host-side tests that run in CI (see [Tests](#tests)).

## Why HTTP REST instead of WebSockets?

E-paper refreshes take roughly 1–4 seconds and updates are inherently
low-frequency, so there is no streaming workload that would benefit from a
persistent socket. A stateless HTTP API is lighter on the ESP32, simpler to
reason about, and callable from `curl`, a browser, or any language without a
client library. WebSockets would add cost and complexity for no gain here.

## Hardware

| Signal | ESP32-S3 pin (default) |
|--------|------------------------|
| MOSI   | 11 |
| SCLK   | 12 |
| CS     | 10 |
| DC     | 13 |
| RST    | 14 |
| BUSY   | 4  |

Pins and panel are configured in [`config.h`](config.h). If you use a different
GxEPD2-supported panel, change the driver class in
[`smart-eink-display.ino`](smart-eink-display.ino) and the width/height in `config.h`.

## Compiling & flashing the firmware

### Prerequisites (one-time)

1. Install the **ESP32 board support** package (Espressif).
2. Install these libraries:
   - `GxEPD2` (pulls in `Adafruit GFX`)
   - `ArduinoJson` (v7)
3. Edit [`config.h`](config.h): set `WIFI_SSID` / `WIFI_PASSWORD`, and optionally
   `DEVICE_NAME`. The pin map already matches this build's wiring.

### Option A — Arduino IDE

1. Open `smart-eink-display.ino` (the whole folder must stay together — the IDE
   compiles `smart-eink-display.ino`, `config.h`, and everything under `src/`; the
   `test/` folder is ignored).
2. **Tools ▸ Board ▸ esp32 ▸ "ESP32S3 Dev Module"**.
3. If your board has PSRAM, set **Tools ▸ PSRAM ▸ "OPI PSRAM"** (or "QSPI PSRAM"
   to match your module).
4. Select the correct **Port**.
5. Click **Upload** (→). To recompile without flashing, use **Verify** (✓).
6. Open **Serial Monitor** at **115200 baud** to see the assigned IP address.

### Option B — arduino-cli

```bash
# One-time setup
arduino-cli core install esp32:esp32
arduino-cli lib install "GxEPD2" "ArduinoJson"

# Compile only (from the folder that contains smart-eink-display.ino)
arduino-cli compile --fqbn esp32:esp32:esp32s3 .

# Compile + flash (replace the port; e.g. COM5 on Windows, /dev/ttyUSB0 on Linux)
arduino-cli compile --fqbn esp32:esp32:esp32s3 --port COM5 --upload .

# Watch serial output for the IP address
arduino-cli monitor --port COM5 --config baudrate=115200
```

If your board needs PSRAM enabled, append it to the FQBN, e.g.
`--fqbn esp32:esp32:esp32s3:PSRAM=opi`.

### If the upload fails

Some ESP32-S3 boards need to be put into the bootloader manually: hold **BOOT**,
tap **RESET**, release **BOOT**, then upload. After a successful flash, press
**RESET** once to run the new firmware.

## HTTP API

Base URL: `http://<ip>/` or `http://eink-display.local/`

| Method & path      | Body | Description |
|--------------------|------|-------------|
| `GET  /`           | —    | Web dashboard |
| `GET  /api/status` | —    | JSON health/status |
| `POST /api/text`   | JSON | Render text (word-wrapped) |
| `POST /api/clear`  | —    | Blank the screen to white |
| `POST /api/image`  | raw  | Render a 1-bit BMP (request body = file bytes) |
| `POST /api/status` | —    | Redraw the boot/status screen |

### `POST /api/text`

```json
{ "title": "Standup", "lines": ["9:30 AM", "Room 4"], "size": "medium" }
```

- `title` *(optional)* — bold heading with an underline.
- `lines` — array of strings; each is word-wrapped to the panel width.
  Alternatively send `"text": "single string"`.
- `size` — `"small"`, `"medium"` (default), or `"large"`.

### `POST /api/image` — sending pictures

The panel is 1-bit black/white, so an image must be converted to an
**uncompressed 1 bit-per-pixel BMP**, at most 800×480 (smaller images are
centered). The device rejects anything else with a JSON error, so conversion
happens on your computer, not on the ESP32.

**Convert with [ImageMagick](https://imagemagick.org):**

```bash
# Resize to fit, dither to black/white, write an uncompressed 1-bit BMP.
magick input.png \
  -resize 800x480 \
  -dither FloydSteinberg -monochrome \
  -type bilevel -compress none \
  BMP3:output.bmp
```

- `BMP3:` forces the classic Windows v3 (BITMAPINFOHEADER) format the parser
  expects; `-compress none` keeps it uncompressed; `-type bilevel` makes it
  1 bit-per-pixel.
- Floyd–Steinberg dithering approximates greyscale with black/white dots; drop
  `-dither FloydSteinberg` for hard thresholding (better for line art/logos).

**Then upload the resulting file:**

```bash
curl -X POST http://eink-display.local/api/image \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @output.bmp
```

On success you get `{"status":"ok","bytes":<n>}`; on a bad file, a `400` with the
reason (e.g. `only 1 bit-per-pixel BMP is supported`).

### Examples

```bash
# Text
curl -X POST http://eink-display.local/api/text \
  -H 'Content-Type: application/json' \
  -d '{"title":"Hello","lines":["Line one","Line two"],"size":"large"}'

# Status
curl http://eink-display.local/api/status

# Clear
curl -X POST http://eink-display.local/api/clear

# Image (must be an uncompressed 1-bit, 800x480-or-smaller BMP)
curl -X POST http://eink-display.local/api/image \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @picture.bmp
```

`GET /api/status` returns, for example:

```json
{"name":"eink-display","ip":"192.168.1.5","ssid":"MyNet","wifiConnected":true,
 "apMode":false,"rssi":-55,"width":800,"height":480,"aspectRatio":"5:3",
 "freeHeap":204800,"uptimeSec":42,"firmware":"1.0.0","lastCommand":"text"}
```

## Project layout

```
smart-eink-display.ino   Main sketch: hardware setup, rendering, HTTP handlers
config.h                 User configuration (WiFi, pins, panel size)
src/layout.{h,cpp}       Pure: aspect-ratio + word-wrap helpers
src/bmp.{h,cpp}          Pure: 1-bit BMP parse/validate + pixel lookup
src/status.{h,cpp}       Pure: status → JSON serialization
test/                    Host-side unit tests (no hardware needed)
.github/workflows/       CI that builds and runs the tests
```

The `src/` modules deliberately avoid any Arduino/hardware dependency so the
same code compiles on the ESP32 **and** natively for unit testing.

## Tests

The logic most worth testing — aspect-ratio math, text wrapping, BMP parsing,
and JSON serialization — lives in `src/` as plain C++ and is exercised by the
suites in `test/`.

Requires a C++17 compiler (g++, clang++, or MSVC).

```bash
# Linux / macOS / MinGW
cd test && make

# Windows (PowerShell)
pwsh test/run_tests.ps1
```

They also run automatically on every push via GitHub Actions
([`.github/workflows/tests.yml`](.github/workflows/tests.yml)).
