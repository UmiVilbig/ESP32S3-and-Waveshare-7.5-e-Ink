#include "bmp.h"

namespace bmp {

namespace {

uint16_t readU16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

uint32_t readU32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

int32_t readS32(const uint8_t* p) {
  return static_cast<int32_t>(readU32(p));
}

// Offsets within the combined file + info header.
constexpr size_t kFileHeaderSize = 14;
constexpr size_t kMinInfoHeader = 40;  // BITMAPINFOHEADER
constexpr size_t kPaletteOffset = kFileHeaderSize + kMinInfoHeader;  // 54

}  // namespace

BmpInfo parseBmp(const uint8_t* data, size_t len) {
  BmpInfo info;
  if (data == nullptr || len < kFileHeaderSize + kMinInfoHeader) {
    info.error = "too small for a BMP header";
    return info;
  }
  if (data[0] != 'B' || data[1] != 'M') {
    info.error = "missing 'BM' magic";
    return info;
  }

  uint32_t dataOffset = readU32(data + 10);
  uint32_t headerSize = readU32(data + 14);
  int32_t width = readS32(data + 18);
  int32_t height = readS32(data + 22);
  uint16_t bpp = readU16(data + 28);
  uint32_t compression = readU32(data + 30);

  if (headerSize < kMinInfoHeader) {
    info.error = "unsupported (pre-3.0) BMP header";
    return info;
  }
  if (compression != 0) {
    info.error = "compressed BMP not supported";
    return info;
  }
  if (bpp != 1) {
    info.error = "only 1 bit-per-pixel BMP is supported";
    return info;
  }
  if (width <= 0) {
    info.error = "invalid width";
    return info;
  }
  if (height == 0) {
    info.error = "invalid height";
    return info;
  }

  info.topDown = height < 0;
  int32_t absHeight = info.topDown ? -height : height;
  uint32_t rowSize = (((static_cast<uint32_t>(bpp) * width) + 31u) / 32u) * 4u;

  // Ensure the pixel data actually fits in the buffer we were handed.
  uint64_t needed =
      static_cast<uint64_t>(dataOffset) + static_cast<uint64_t>(rowSize) *
                                              static_cast<uint64_t>(absHeight);
  if (dataOffset < kPaletteOffset || needed > len) {
    info.error = "pixel data runs past end of buffer";
    return info;
  }

  info.valid = true;
  info.error = "";
  info.width = width;
  info.height = absHeight;
  info.bitsPerPixel = bpp;
  info.dataOffset = dataOffset;
  info.rowSize = rowSize;
  return info;
}

bool pixelBlack(const uint8_t* data, size_t len, const BmpInfo& info, int x,
                int y) {
  if (!info.valid || data == nullptr) return false;
  if (x < 0 || y < 0 || x >= info.width || y >= info.height) return false;

  // Bottom-up BMPs store the first row last.
  int srcRow = info.topDown ? y : (info.height - 1 - y);
  uint32_t byteIndex =
      info.dataOffset + static_cast<uint32_t>(srcRow) * info.rowSize +
      static_cast<uint32_t>(x / 8);
  if (byteIndex >= len) return false;

  int bit = (data[byteIndex] >> (7 - (x % 8))) & 1;

  // Decide colour from the 2-entry palette (BGRA per entry).
  uint32_t palEntry = kPaletteOffset + static_cast<uint32_t>(bit) * 4u;
  if (palEntry + 3u >= len) {
    // No usable palette: fall back to the common convention (0 = black).
    return bit == 0;
  }
  int blue = data[palEntry];
  int green = data[palEntry + 1];
  int red = data[palEntry + 2];
  int luminance = (red + green + blue) / 3;
  return luminance < 128;
}

}  // namespace bmp
