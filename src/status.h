// status.h — pure, hardware-independent status/JSON helpers.
//
// The .ino gathers live values (IP, RSSI, heap, ...) from the hardware and
// hands them to these functions, which build the JSON served at /api/status.
// Keeping the formatting here (with no Arduino types) makes it unit-testable.
#pragma once

#include <cstdint>
#include <string>

namespace status {

struct DeviceStatus {
  std::string name;
  std::string ip;
  std::string ssid;
  bool wifiConnected = false;
  bool apMode = false;      // true when running as a SoftAP fallback
  int rssi = 0;             // dBm (meaningful only when wifiConnected)
  int width = 0;            // display pixels
  int height = 0;           // display pixels
  uint32_t freeHeap = 0;    // bytes
  uint32_t uptimeSec = 0;   // seconds since boot
  std::string firmware;
  std::string lastCommand;  // last render action performed
  std::string lastError;    // most recent error, empty when none
};

// Escape a string for embedding inside a JSON double-quoted value.
std::string jsonEscape(const std::string& s);

// Render the status as a compact JSON object with a stable key order.
std::string toJson(const DeviceStatus& s);

}  // namespace status
