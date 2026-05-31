#include <GxEPD2_BW.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Fonts/FreeSansBold18pt7b.h>

#define EPD_MOSI  11
#define EPD_CLK   12
#define EPD_CS    10
#define EPD_DC    13
#define EPD_RST   14
#define EPD_BUSY   4

#define EPD_WIDTH  800
#define EPD_HEIGHT 480
#define BMP_ROW_SIZE (((EPD_WIDTH + 31) / 32) * 4)
#define MAX_LINES 10

const char* ssid     = "Viltronics";
const char* password = "1A2B3C4D5E";

SPIClass fspi(FSPI);
WiFiServer tcpServer(8080);
WebServer http(80);

GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT / 2> display(
  GxEPD2_750_GDEY075T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

void displayBitmap(uint8_t* bmpData, int len) {
  uint32_t pixelOffset = bmpData[10] | (bmpData[11] << 8) |
                         (bmpData[12] << 16) | (bmpData[13] << 24);

  Serial.println("Pixel offset: " + String(pixelOffset));
  Serial.println("Rendering bitmap...");

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
  Serial.println("Bitmap done!");
}

void displayText(String lines[], int count) {
  Serial.println("Rendering " + String(count) + " text lines...");

  display.setFullWindow();
  display.setFont(&FreeSansBold18pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    int y = 60;
    for (int i = 0; i < count && y < EPD_HEIGHT - 20; i++) {
      display.setCursor(40, y);
      display.print(lines[i]);
      y += 50;
    }
  } while (display.nextPage());

  display.hibernate();
  Serial.println("Text done!");
}

int parseJsonLines(const String& body, String out[], int maxLines) {
  int count = 0;
  int arrStart = body.indexOf('[');
  int arrEnd = body.lastIndexOf(']');
  if (arrStart < 0 || arrEnd <= arrStart) return 0;

  int pos = arrStart + 1;
  while (pos < arrEnd && count < maxLines) {
    int q1 = body.indexOf('"', pos);
    if (q1 < 0 || q1 >= arrEnd) break;
    int q2 = q1 + 1;
    while (q2 < arrEnd) {
      q2 = body.indexOf('"', q2);
      if (q2 < 0) break;
      if (q2 > 0 && body[q2 - 1] == '\\') { q2++; continue; }
      break;
    }
    if (q2 < 0) break;
    out[count++] = body.substring(q1 + 1, q2);
    pos = q2 + 1;
  }
  return count;
}

void handleDisplay() {
  String lines[MAX_LINES];
  int lineCount = 0;

  for (int i = 0; i < http.args() && lineCount < MAX_LINES; i++) {
    if (http.argName(i) == "text" || http.argName(i) == "line") {
      lines[lineCount++] = http.arg(i);
    }
  }

  if (lineCount == 0 && http.hasArg("plain")) {
    String body = http.arg("plain");
    body.trim();
    if (body.startsWith("{")) {
      lineCount = parseJsonLines(body, lines, MAX_LINES);
    } else {
      int pos = 0;
      while (pos <= (int)body.length() && lineCount < MAX_LINES) {
        int nl = body.indexOf('\n', pos);
        if (nl < 0) nl = body.length();
        String line = body.substring(pos, nl);
        line.trim();
        if (line.length() > 0) lines[lineCount++] = line;
        pos = nl + 1;
      }
    }
  }

  if (lineCount > 0) {
    displayText(lines, lineCount);
    http.send(200, "application/json",
      "{\"status\":\"ok\",\"lines\":" + String(lineCount) + "}");
  } else {
    http.send(400, "application/json",
      "{\"error\":\"No text provided. Use ?text=Hello or POST {\\\"lines\\\":[\\\"Hello\\\"]}\"}");
  }
}

void handleRoot() {
  if (http.method() == HTTP_POST) {
    handleDisplay();
    return;
  }
  http.send(200, "application/json",
    "{\"name\":\"E-Ink Display\","
    "\"endpoints\":{"
    "\"GET /display?text=Hello\":\"display text via query params\","
    "\"POST /display\":\"JSON {\\\"lines\\\":[...]} or plain text body\","
    "\"POST /\":\"same as POST /display\","
    "\"TCP :8080\":\"binary BMP protocol (4-byte length header + BMP data)\"}}");
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

  http.on("/", handleRoot);
  http.on("/display", HTTP_GET, handleDisplay);
  http.on("/display", HTTP_POST, handleDisplay);
  http.begin();
  Serial.println("HTTP server on port 80");

  tcpServer.begin();
  Serial.println("TCP server on port 8080 — waiting for BMP data...");
}

void loop() {
  http.handleClient();

  WiFiClient client = tcpServer.available();
  if (!client) return;

  Serial.println("TCP client connected");

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

  if (dataLen > 100000) {
    if (header[0] >= 'A' && header[0] <= 'Z') {
      // HTTP request hit the binary TCP port — send a helpful redirect
      timeout = millis() + 2000;
      while (client.available() && millis() < timeout) client.read();
      String ip = WiFi.localIP().toString();
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("Wrong port. Use HTTP on port 80 for text display:");
      client.println("  GET  http://" + ip + "/display?text=Hello");
      client.println("  POST http://" + ip + "/display");
      client.println("  Body: {\"lines\":[\"Hello\",\"World\"]}");
      client.println("Port 8080 only accepts binary BMP data.");
      Serial.println("HTTP request redirected to port 80");
    } else {
      Serial.println("Invalid size: " + String(dataLen));
    }
    client.stop();
    return;
  }

  Serial.println("Expecting " + String(dataLen) + " bytes");

  uint8_t* bmpData = (uint8_t*)ps_malloc(dataLen);
  if (!bmpData) bmpData = (uint8_t*)malloc(dataLen);
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

  Serial.println("Received " + String(bytesRead) + "/" + String(dataLen) + " bytes");

  if (bytesRead < dataLen) {
    Serial.println("Incomplete transfer!");
    client.print("ERR:incomplete:" + String(bytesRead) + "/" + String(dataLen));
    client.stop();
    free(bmpData);
    return;
  }

  if (bytesRead < 54) {
    Serial.println("Not enough data for BMP header");
    client.print("ERR:too_small");
    client.stop();
    free(bmpData);
    return;
  }

  displayBitmap(bmpData, bytesRead);
  client.print("OK:" + String(bytesRead));
  client.stop();
  free(bmpData);
}
