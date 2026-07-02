#include "../src/status.h"

#include "test_framework.h"

TEST(json_escape_specials) {
  CHECK_EQ(status::jsonEscape("plain"), std::string("plain"));
  CHECK_EQ(status::jsonEscape("a\"b"), std::string("a\\\"b"));
  CHECK_EQ(status::jsonEscape("a\\b"), std::string("a\\\\b"));
  CHECK_EQ(status::jsonEscape("a\nb"), std::string("a\\nb"));
  CHECK_EQ(status::jsonEscape("a\tb"), std::string("a\\tb"));
}

TEST(status_json_full) {
  status::DeviceStatus s;
  s.name = "eink-display";
  s.ip = "192.168.1.5";
  s.ssid = "MyNet";
  s.wifiConnected = true;
  s.apMode = false;
  s.rssi = -55;
  s.width = 800;
  s.height = 480;
  s.freeHeap = 204800;
  s.uptimeSec = 42;
  s.firmware = "1.0.0";
  s.lastCommand = "text";

  const std::string expected =
      "{\"name\":\"eink-display\",\"ip\":\"192.168.1.5\",\"ssid\":\"MyNet\","
      "\"wifiConnected\":true,\"apMode\":false,\"rssi\":-55,\"width\":800,"
      "\"height\":480,\"aspectRatio\":\"5:3\",\"freeHeap\":204800,"
      "\"uptimeSec\":42,\"firmware\":\"1.0.0\",\"lastCommand\":\"text\","
      "\"lastError\":\"\"}";
  CHECK_EQ(status::toJson(s), expected);
}

TEST(status_json_escapes_ssid) {
  status::DeviceStatus s;
  s.name = "d";
  s.ip = "0.0.0.0";
  s.ssid = "Cafe \"Wifi\"";
  s.firmware = "1.0.0";
  s.width = 800;
  s.height = 480;
  std::string j = status::toJson(s);
  // The embedded quotes must be escaped in the output.
  CHECK(j.find("\"ssid\":\"Cafe \\\"Wifi\\\"\"") != std::string::npos);
  CHECK(j.find("\"apMode\":false") != std::string::npos);
}
