// =============================================================================
// smart-eink-display — ESP32-S3 networked e-paper display
//
// A polished, flashable Arduino sketch for a 7.5" 800x480 black/white e-paper
// panel (GoodDisplay/Waveshare GDEY075T7) driven over SPI with GxEPD2.
//
// On boot the panel shows its own health/status (host name, IP, WiFi state,
// resolution + aspect ratio, firmware, free memory). It then serves a small
// JSON REST API over HTTP so you can push text, clear the screen, upload a
// 1-bit BMP image, or query status.
//
// Why REST (not WebSocket)? E-paper refreshes take ~1-4 s and updates are
// low-frequency by nature, so there is no streaming workload to justify a
// persistent socket. A stateless HTTP API is lighter, simpler, and works from
// curl, a browser, or any language with zero client library.
//
//   GET  /              -> HTML dashboard + live status
//   GET  /api/status    -> JSON health/status
//   POST /api/text      -> {"title":"..","lines":["..",".."],"size":"medium"}
//   POST /api/clear     -> blank the screen to white
//   POST /api/image     -> raw 1-bit BMP in the request body
//   POST /api/status    -> redraw the boot/status screen
//
// Dependencies (install via Arduino Library Manager):
//   - GxEPD2        (pulls in Adafruit GFX)
//   - ArduinoJson   (v7)
//
// Board: "ESP32S3 Dev Module" (or your S3 variant). See README.md.
// =============================================================================

#include <Adafruit_GFX.h>  // must precede the <Fonts/...> includes so the
                           // Arduino builder adds the GFX library to the
                           // include path before resolving the font headers.
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "src/bmp.h"
#include "src/layout.h"
#include "src/status.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
SPIClass fspi(FSPI);
WebServer http(HTTP_PORT);

// GDEY075T7 (800x480). The template's second argument is the page height; using
// HEIGHT/2 keeps RAM use reasonable by rendering the screen in two passes.
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT / 2> display(
    GxEPD2_750_GDEY075T7(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

bool g_apMode = false;
String g_lastCommand = "boot";
String g_lastError = "";  // most recent error, "" when none; shown on panel

// Map a size name to a font plus rough per-glyph metrics used for wrapping.
// Defined up here (before the first function) so the Arduino IDE's auto-
// generated prototypes can see the type when it prototypes pickFont().
struct FontChoice {
  const GFXfont* font;
  int charWidth;   // approximate advance width in px
  int lineHeight;  // px between baselines
};

// ---------------------------------------------------------------------------
// Status helpers
// ---------------------------------------------------------------------------
status::DeviceStatus gatherStatus() {
  status::DeviceStatus s;
  s.name = DEVICE_NAME;
  s.apMode = g_apMode;
  if (g_apMode) {
    s.ip = WiFi.softAPIP().toString().c_str();
    s.ssid = DEVICE_NAME;
    s.wifiConnected = false;
    s.rssi = 0;
  } else {
    s.wifiConnected = WiFi.status() == WL_CONNECTED;
    s.ssid = WiFi.SSID().c_str();
    s.rssi = WiFi.RSSI();
    // localIP() returns 0.0.0.0 when there is no DHCP lease; leave the field
    // empty in that case so the UI can say "not connected" instead of showing
    // a meaningless address.
    s.ip = s.wifiConnected ? WiFi.localIP().toString().c_str() : "";
  }
  s.width = DISPLAY_WIDTH;
  s.height = DISPLAY_HEIGHT;
  s.freeHeap = ESP.getFreeHeap();
  s.uptimeSec = millis() / 1000;
  s.firmware = FIRMWARE_VERSION;
  s.lastCommand = g_lastCommand.c_str();
  s.lastError = g_lastError.c_str();
  return s;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

FontChoice pickFont(const String& size) {
  if (size == "small") return {&FreeSans9pt7b, 9, 26};
  if (size == "large") return {&FreeSansBold18pt7b, 17, 46};
  return {&FreeSansBold12pt7b, 12, 34};  // "medium" (default)
}

// Render a block of text, word-wrapped to the panel width.
void renderText(const String& title, const std::vector<std::string>& rawLines,
                const String& size) {
  FontChoice fc = pickFont(size);
  const int marginX = 30;
  const int usableW = DISPLAY_WIDTH - 2 * marginX;
  const int charsPerLine = usableW / fc.charWidth;

  // Wrap each requested line independently, then flatten.
  std::vector<std::string> wrapped;
  for (const std::string& ln : rawLines) {
    std::vector<std::string> w = layout::wrapText(ln, charsPerLine);
    if (w.empty()) {
      wrapped.push_back("");  // preserve intentional blank lines
    } else {
      wrapped.insert(wrapped.end(), w.begin(), w.end());
    }
  }

  display.setFullWindow();
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    int y = 50;
    if (title.length() > 0) {
      display.setFont(&FreeSansBold18pt7b);
      display.setCursor(marginX, y);
      display.print(title);
      display.drawFastHLine(marginX, y + 10, usableW, GxEPD_BLACK);
      y += 60;
    }
    display.setFont(fc.font);
    for (const std::string& ln : wrapped) {
      if (y > DISPLAY_HEIGHT - 10) break;
      display.setCursor(marginX, y);
      display.print(ln.c_str());
      y += fc.lineHeight;
    }
  } while (display.nextPage());
  display.hibernate();
}

// Draw the boot/status screen from a status snapshot.
void renderStatusScreen(const status::DeviceStatus& s) {
  const int marginX = 30;

  std::string wifiLine;
  if (s.apMode) {
    wifiLine = "WiFi:  SoftAP \"" + s.ssid + "\" (open, no internet)";
  } else if (s.wifiConnected) {
    wifiLine = "WiFi:  " + s.ssid + "  (" + std::to_string(s.rssi) + " dBm)";
  } else {
    wifiLine = "WiFi:  disconnected";
  }

  std::string ipLine =
      s.ip.empty() ? "IP:      not connected" : "IP:      " + s.ip;

  std::vector<std::string> rows = {
      ipLine,
      "Host:    " + s.name + ".local",
      wifiLine,
      "Screen:  " + std::to_string(s.width) + " x " +
          std::to_string(s.height) + "   (" +
          layout::aspectRatio(s.width, s.height) + ")",
      "Memory:  " + std::to_string(s.freeHeap / 1024) + " KB free",
      "Firmware: v" + s.firmware,
  };
  if (!s.lastError.empty()) {
    rows.push_back("Error:   " + s.lastError);
  }

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Title bar.
    display.fillRect(0, 0, DISPLAY_WIDTH, 70, GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(marginX, 48);
    display.print(s.name.c_str());

    // Body rows.
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold12pt7b);
    int y = 130;
    for (const std::string& row : rows) {
      display.setCursor(marginX, y);
      display.print(row.c_str());
      y += 46;
    }

    // Footer hint.
    display.setFont(&FreeSans9pt7b);
    display.setCursor(marginX, DISPLAY_HEIGHT - 24);
    display.print(s.wifiConnected || s.apMode
                      ? "Ready  -  POST /api/text  |  GET / for dashboard"
                      : "Waiting for network...");
  } while (display.nextPage());
  display.hibernate();
}

void renderClear() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
  display.hibernate();
}

// Draw a full-screen error message so failures are visible on the panel
// itself, not only in the HTTP/JSON response.
void renderError(const String& context, const String& message) {
  const int marginX = 30;
  std::vector<std::string> wrapped =
      layout::wrapText(std::string(message.c_str()), 44);

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Inverted title bar reading "ERROR".
    display.fillRect(0, 0, DISPLAY_WIDTH, 70, GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(marginX, 48);
    display.print("ERROR");

    display.setTextColor(GxEPD_BLACK);
    int y = 130;
    if (context.length() > 0) {
      display.setFont(&FreeSansBold12pt7b);
      display.setCursor(marginX, y);
      display.print(context);
      y += 44;
    }

    display.setFont(&FreeSans9pt7b);
    for (const std::string& ln : wrapped) {
      if (y > DISPLAY_HEIGHT - 20) break;
      display.setCursor(marginX, y);
      display.print(ln.c_str());
      y += 26;
    }
  } while (display.nextPage());
  display.hibernate();
}

// Render a parsed 1-bit BMP, centered on the panel.
bool renderBmp(const uint8_t* data, size_t len, String& errorOut) {
  bmp::BmpInfo info = bmp::parseBmp(data, len);
  if (!info.valid) {
    errorOut = info.error;
    return false;
  }
  int offsetX = (DISPLAY_WIDTH - info.width) / 2;
  int offsetY = (DISPLAY_HEIGHT - info.height) / 2;
  if (offsetX < 0) offsetX = 0;
  if (offsetY < 0) offsetY = 0;

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    for (int y = 0; y < info.height && (y + offsetY) < DISPLAY_HEIGHT; y++) {
      for (int x = 0; x < info.width && (x + offsetX) < DISPLAY_WIDTH; x++) {
        if (bmp::pixelBlack(data, len, info, x, y)) {
          display.drawPixel(x + offsetX, y + offsetY, GxEPD_BLACK);
        }
      }
    }
  } while (display.nextPage());
  display.hibernate();
  return true;
}

// ---------------------------------------------------------------------------
// HTTP handlers
// ---------------------------------------------------------------------------
void sendJson(int code, const String& body) {
  http.sendHeader("Access-Control-Allow-Origin", "*");
  http.send(code, "application/json", body);
}

// Report a command failure: remember it, show it on the panel, and return the
// JSON error. `context` is a short label (e.g. "POST /api/text").
void fail(int code, const String& context, const String& message) {
  g_lastError = message;
  g_lastCommand = "error";
  renderError(context, message);
  sendJson(code, String("{\"error\":\"") +
                     status::jsonEscape(message.c_str()).c_str() + "\"}");
}

void handleStatusGet() {
  sendJson(200, String(status::toJson(gatherStatus()).c_str()));
}

void handleStatusScreen() {
  g_lastCommand = "status";
  renderStatusScreen(gatherStatus());
  sendJson(200, "{\"status\":\"ok\",\"action\":\"status-screen\"}");
}

void handleText() {
  if (!http.hasArg("plain")) {
    fail(400, "POST /api/text", "empty body; expected JSON");
    return;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.arg("plain"));
  if (err) {
    fail(400, "POST /api/text", String("invalid JSON: ") + err.c_str());
    return;
  }

  String title = doc["title"] | "";
  String size = doc["size"] | "medium";

  std::vector<std::string> lines;
  if (doc["lines"].is<JsonArray>()) {
    for (JsonVariant v : doc["lines"].as<JsonArray>()) {
      lines.push_back(std::string(v.as<const char*>() ? v.as<const char*>()
                                                      : ""));
    }
  } else if (doc["text"].is<const char*>()) {
    lines.push_back(std::string(doc["text"].as<const char*>()));
  }

  if (lines.empty() && title.length() == 0) {
    fail(400, "POST /api/text",
         "provide 'lines':[...], 'text':'..', or 'title'");
    return;
  }

  g_lastCommand = "text";
  g_lastError = "";
  renderText(title, lines, size);
  sendJson(200, String("{\"status\":\"ok\",\"lines\":") +
                    String((int)lines.size()) + "}");
}

void handleClear() {
  g_lastCommand = "clear";
  g_lastError = "";
  renderClear();
  sendJson(200, "{\"status\":\"ok\",\"action\":\"clear\"}");
}

void handleImage() {
  if (!http.hasArg("plain")) {
    fail(400, "POST /api/image", "empty body; POST raw 1-bit BMP bytes");
    return;
  }
  const String& body = http.arg("plain");
  if (body.length() > MAX_IMAGE_BYTES) {
    fail(413, "POST /api/image", "image too large");
    return;
  }
  String errorOut;
  bool ok = renderBmp(reinterpret_cast<const uint8_t*>(body.c_str()),
                      body.length(), errorOut);
  if (!ok) {
    fail(400, "POST /api/image", errorOut);
    return;
  }
  g_lastCommand = "image";
  g_lastError = "";
  sendJson(200, String("{\"status\":\"ok\",\"bytes\":") +
                    String((int)body.length()) + "}");
}

void handleRoot() {
  // Compact dashboard: shows live status and a form to push text.
  String html =
      "<!doctype html><html><head><meta charset=utf-8>"
      "<meta name=viewport content='width=device-width,initial-scale=1'>"
      "<title>" DEVICE_NAME "</title><style>"
      "body{font-family:system-ui,sans-serif;max-width:640px;margin:2rem auto;"
      "padding:0 1rem;color:#111}h1{margin:0 0 .25rem}"
      "textarea,input,button,select{font:inherit;width:100%;box-sizing:border-box;"
      "margin:.25rem 0;padding:.5rem}button{cursor:pointer}"
      "pre{background:#f4f4f4;padding:1rem;border-radius:6px;overflow:auto}"
      ".row{display:flex;gap:.5rem}</style></head><body>"
      "<h1>" DEVICE_NAME "</h1><p>ESP32-S3 e-paper display</p>"
      "<h3>Send text</h3>"
      "<input id=title placeholder='Title (optional)'>"
      "<textarea id=lines rows=4 placeholder='One line per row'></textarea>"
      "<div class=row><select id=size>"
      "<option>small</option><option selected>medium</option>"
      "<option>large</option></select>"
      "<button onclick=sendText()>Display</button>"
      "<button onclick=clr()>Clear</button></div>"
      "<h3>Status</h3><pre id=st>loading...</pre>"
      "<script>"
      "async function refresh(){let r=await fetch('/api/status');"
      "document.getElementById('st').textContent="
      "JSON.stringify(await r.json(),null,2);}"
      "async function sendText(){let lines=document.getElementById('lines')"
      ".value.split('\\n');await fetch('/api/text',{method:'POST',"
      "headers:{'Content-Type':'application/json'},body:JSON.stringify({"
      "title:document.getElementById('title').value,lines:lines,"
      "size:document.getElementById('size').value})});refresh();}"
      "async function clr(){await fetch('/api/clear',{method:'POST'});refresh();}"
      "refresh();setInterval(refresh,10000);"
      "</script></body></html>";
  http.send(200, "text/html", html);
}

void handleNotFound() {
  // Answer CORS preflight so cross-origin browser clients can POST.
  if (http.method() == HTTP_OPTIONS) {
    http.sendHeader("Access-Control-Allow-Origin", "*");
    http.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    http.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    http.send(204);
    return;
  }
  sendJson(404, "{\"error\":\"not found\",\"see\":\"GET /\"}");
}

// ---------------------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------------------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(DEVICE_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long deadline = millis() + WIFI_TIMEOUT_MS;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    g_apMode = false;
    Serial.println("Connected. IP: " + WiFi.localIP().toString());
  } else {
    g_apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(DEVICE_NAME);
    Serial.println("WiFi failed; started SoftAP \"" DEVICE_NAME "\" at " +
                   WiFi.softAPIP().toString());
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nsmart-eink-display v" FIRMWARE_VERSION " booting...");

  fspi.begin(EPD_CLK, -1, EPD_MOSI, EPD_CS);
  display.init(115200, true, 20, false, fspi,
               SPISettings(1000000, MSBFIRST, SPI_MODE0));
  Serial.println("Display ready");

  connectWiFi();

  if (!g_apMode && MDNS.begin(DEVICE_NAME)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.println("mDNS: http://" DEVICE_NAME ".local/");
  }

  http.on("/", HTTP_GET, handleRoot);
  http.on("/api/status", HTTP_GET, handleStatusGet);
  http.on("/api/status", HTTP_POST, handleStatusScreen);
  http.on("/api/text", HTTP_POST, handleText);
  http.on("/api/clear", HTTP_POST, handleClear);
  http.on("/api/image", HTTP_POST, handleImage);
  http.onNotFound(handleNotFound);
  http.begin();
  Serial.println("HTTP server started on port " + String(HTTP_PORT));

  renderStatusScreen(gatherStatus());
  Serial.println("Boot status shown on panel");
}

void loop() {
  http.handleClient();
}
