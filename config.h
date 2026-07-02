// config.h — user-editable configuration.
//
// Copy your WiFi credentials in below and adjust the pin map to match how your
// e-paper HAT is wired to the ESP32-S3. Everything hardware-specific lives here
// so the rest of the sketch stays generic.
#pragma once

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------
#define WIFI_SSID      "Viltronics"
#define WIFI_PASSWORD  "1A2B3C4D5E"

// How long to wait for WiFi before falling back to a SoftAP (milliseconds).
#define WIFI_TIMEOUT_MS  20000

// mDNS/host name. The device will be reachable at http://<DEVICE_NAME>.local/
// If WiFi fails, a SoftAP is started with this name (open network).
#define DEVICE_NAME  "eink-display"

// ---------------------------------------------------------------------------
// Firmware
// ---------------------------------------------------------------------------
#define FIRMWARE_VERSION  "1.0.0"

// ---------------------------------------------------------------------------
// HTTP server
// ---------------------------------------------------------------------------
#define HTTP_PORT  80

// Largest image body we will accept (bytes). An 800x480 1-bit BMP is ~48 KB.
#define MAX_IMAGE_BYTES  120000

// ---------------------------------------------------------------------------
// Display: GoodDisplay/Waveshare 7.5" GDEY075T7, 800x480, black/white.
// If you use a different panel, change the driver class in smart-eink-display.ino and
// the width/height below.
// ---------------------------------------------------------------------------
#define DISPLAY_WIDTH   800
#define DISPLAY_HEIGHT  480

// SPI pin map (ESP32-S3). Adjust to your wiring.
#define EPD_MOSI  11
#define EPD_CLK   12
#define EPD_CS    10
#define EPD_DC    13
#define EPD_RST   14
#define EPD_BUSY   4
