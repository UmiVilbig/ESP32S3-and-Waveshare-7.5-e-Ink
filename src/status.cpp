#include "status.h"

#include "layout.h"

namespace status {

std::string jsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          static const char* hex = "0123456789abcdef";
          out += "\\u00";
          out += hex[(c >> 4) & 0xF];
          out += hex[c & 0xF];
        } else {
          out += c;
        }
    }
  }
  return out;
}

namespace {

std::string kv(const std::string& key, const std::string& value) {
  return "\"" + key + "\":\"" + jsonEscape(value) + "\"";
}

std::string kvRaw(const std::string& key, const std::string& value) {
  return "\"" + key + "\":" + value;
}

}  // namespace

std::string toJson(const DeviceStatus& s) {
  std::string j = "{";
  j += kv("name", s.name) + ",";
  j += kv("ip", s.ip) + ",";
  j += kv("ssid", s.ssid) + ",";
  j += kvRaw("wifiConnected", s.wifiConnected ? "true" : "false") + ",";
  j += kvRaw("apMode", s.apMode ? "true" : "false") + ",";
  j += kvRaw("rssi", std::to_string(s.rssi)) + ",";
  j += kvRaw("width", std::to_string(s.width)) + ",";
  j += kvRaw("height", std::to_string(s.height)) + ",";
  j += kv("aspectRatio", layout::aspectRatio(s.width, s.height)) + ",";
  j += kvRaw("freeHeap", std::to_string(s.freeHeap)) + ",";
  j += kvRaw("uptimeSec", std::to_string(s.uptimeSec)) + ",";
  j += kv("firmware", s.firmware) + ",";
  j += kv("lastCommand", s.lastCommand) + ",";
  j += kv("lastError", s.lastError);
  j += "}";
  return j;
}

}  // namespace status
