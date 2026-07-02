// bmp.h — pure, hardware-independent 1-bit Windows BMP parser.
//
// Parses/validates uncompressed BMP files (BITMAPINFOHEADER) and answers the
// only question the renderer cares about: "is pixel (x, y) black?". No Arduino
// dependencies, so it is fully unit-testable on a host machine.
#pragma once

#include <cstddef>
#include <cstdint>

namespace bmp {

struct BmpInfo {
  bool valid = false;
  const char* error = "not parsed";
  int32_t width = 0;        // pixels (always positive when valid)
  int32_t height = 0;       // pixels (always positive when valid)
  uint16_t bitsPerPixel = 0;
  uint32_t dataOffset = 0;  // byte offset of pixel data
  uint32_t rowSize = 0;     // bytes per row, padded to a 4-byte boundary
  bool topDown = false;     // true if rows are stored top-to-bottom
};

// Parse and structurally validate a BMP in `data` (`len` bytes). For this
// project a BMP is only usable if it is 1 bit-per-pixel and uncompressed;
// parseBmp() reports .valid == false with a human-readable .error otherwise.
BmpInfo parseBmp(const uint8_t* data, size_t len);

// Return true if pixel (x, y) — origin top-left — is black, using the BMP's
// 2-entry color palette to decide. Out-of-bounds or invalid input returns
// false (treated as white/background).
bool pixelBlack(const uint8_t* data, size_t len, const BmpInfo& info, int x,
                int y);

}  // namespace bmp
