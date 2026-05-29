#include <GxEPD2_BW.h>
#include <SPI.h>
#include <WiFi.h>

#define EPD_MOSI  11
#define EPD_CLK   12
#define EPD_CS    10
#define EPD_DC    13
#define EPD_RST   14
#define EPD_BUSY   4

#define EPD_WIDTH  800
#define EPD_HEIGHT 480
#define BMP_ROW_SIZE (((EPD_WIDTH + 31) / 32) * 4)

const char* ssid     = "Viltronics";
const char* password = "1A2B3C4D5E";

SPIClass fspi(FSPI);
WiFiServer tcpServer(8080);

GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT / 2> display(
  GxEPD2_750_GDEY075T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

void displayBitmap(uint8_t* bmpData, int len) {
  uint32_t pixelOffset = bmpData[10] | (bmpData[11] << 8) |
                         (bmpData[12] << 16) | (bmpData[13] << 24);

  Serial.println("Pixel offset: " + String(pixelOffset));
  Serial.println("Rendering...");

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    for (int y = 0; y < EPD_HEIGHT; y++) {
      int bmpRow = (EPD_HEIGHT - 1 - y);
      uint32_t rowOffset = pixelOffset + bmpRow * BMP_ROW_SIZE;
      for (int x = 0; x < EPD_WIDTH; x++) {
        uint32_t byteIndex = rowOffset + (x / 8);
        if (byteIndex >= (uint32_t)len) continue;
        bool isWhite = (bmpData[byteIndex] >> (7 - (x % 8))) & 1;
        if (!isWhite) display.drawPixel(x, y, GxEPD_BLACK);
      }
    }
  } while (display.nextPage());

  display.hibernate();
  Serial.println("Done!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting...");

  fspi.begin(EPD_CLK, -1, EPD_MOSI, EPD_CS);
  display.init(115200, true, 20, false, fspi, SPISettings(1000000, MSBFIRST, SPI_MODE0));
  Serial.println("Display ready");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  tcpServer.begin();
  Serial.println("TCP server on port 8080 — waiting for data...");
}

void loop() {
  WiFiClient client = tcpServer.available();
  if (!client) return;

  Serial.println("Client connected");

  // First 4 bytes = data length (little-endian uint32)
  uint8_t header[4];
  unsigned long timeout = millis() + 5000;
  int headerRead = 0;
  while (headerRead < 4 && millis() < timeout) {
    if (client.available()) {
      header[headerRead++] = client.read();
    }
  }

  if (headerRead < 4) {
    Serial.println("Failed to read header");
    client.stop();
    return;
  }

  uint32_t dataLen = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);
  if (dataLen > 100000) {  // BMP should be ~48KB, reject anything crazy
    Serial.println("Invalid size: " + String(dataLen));
    client.stop();
    return;
  }
  Serial.println("Expecting " + String(dataLen) + " bytes");

  uint8_t* bmpData = (uint8_t*)ps_malloc(dataLen);  // use PSRAM
  if (!bmpData) {
    bmpData = (uint8_t*)malloc(dataLen);  // fallback to regular RAM
  }
  if (!bmpData) {
    Serial.println("Out of memory!");
    client.stop();
    return;
  }

  uint32_t bytesRead = 0;
  timeout = millis() + 10000;
  while (bytesRead < dataLen && millis() < timeout) {
    if (client.available()) {
      int chunk = client.read(bmpData + bytesRead, dataLen - bytesRead);
      if (chunk > 0) bytesRead += chunk;
    }
  }

  Serial.println("Received " + String(bytesRead) + " bytes");
  client.print("OK");
  client.stop();

  if (bytesRead >= 54) {
    displayBitmap(bmpData, bytesRead);
  } else {
    Serial.println("Not enough data");
  }

  free(bmpData);
}