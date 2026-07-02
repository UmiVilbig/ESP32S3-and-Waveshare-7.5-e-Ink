#include "../src/bmp.h"

#include <cstdint>
#include <vector>

#include "test_framework.h"

namespace {

void putU16(std::vector<uint8_t>& v, uint16_t x) {
  v.push_back(x & 0xFF);
  v.push_back((x >> 8) & 0xFF);
}
void putU32(std::vector<uint8_t>& v, uint32_t x) {
  v.push_back(x & 0xFF);
  v.push_back((x >> 8) & 0xFF);
  v.push_back((x >> 16) & 0xFF);
  v.push_back((x >> 24) & 0xFF);
}

// Build a valid, uncompressed 1-bpp bottom-up BMP whose two-entry palette is
// index 0 = black, index 1 = white. `rows` are given top-to-bottom, each a
// vector of packed bytes already padded to `rowSize`.
std::vector<uint8_t> buildBmp(int32_t width, int32_t height,
                              const std::vector<std::vector<uint8_t>>& rowsTop) {
  const uint32_t dataOffset = 14 + 40 + 8;  // headers + 2 palette entries
  std::vector<uint8_t> v;
  // File header.
  v.push_back('B');
  v.push_back('M');
  putU32(v, 0);           // file size (unchecked by parser)
  putU32(v, 0);           // reserved
  putU32(v, dataOffset);  // pixel data offset
  // Info header (BITMAPINFOHEADER).
  putU32(v, 40);
  putU32(v, static_cast<uint32_t>(width));
  putU32(v, static_cast<uint32_t>(height));
  putU16(v, 1);   // planes
  putU16(v, 1);   // bpp
  putU32(v, 0);   // compression (BI_RGB)
  putU32(v, 0);   // image size
  putU32(v, 0);   // x ppm
  putU32(v, 0);   // y ppm
  putU32(v, 2);   // colors used
  putU32(v, 0);   // important colors
  // Palette: index 0 black, index 1 white (BGRA).
  v.push_back(0); v.push_back(0); v.push_back(0); v.push_back(0);
  v.push_back(0xFF); v.push_back(0xFF); v.push_back(0xFF); v.push_back(0);
  // Pixel data, stored bottom-up.
  for (auto it = rowsTop.rbegin(); it != rowsTop.rend(); ++it) {
    v.insert(v.end(), it->begin(), it->end());
  }
  return v;
}

}  // namespace

TEST(bmp_rejects_garbage) {
  std::vector<uint8_t> tiny = {'B', 'M', 0, 0};
  bmp::BmpInfo info = bmp::parseBmp(tiny.data(), tiny.size());
  CHECK(!info.valid);

  std::vector<uint8_t> noMagic(60, 0);
  bmp::BmpInfo i2 = bmp::parseBmp(noMagic.data(), noMagic.size());
  CHECK(!i2.valid);
}

TEST(bmp_parses_valid_header) {
  // 8x2 image: row0 all black (0x00), row1 all white (0xFF); padded to 4 bytes.
  auto buf = buildBmp(8, 2,
                      {{0x00, 0, 0, 0}, {0xFF, 0, 0, 0}});
  bmp::BmpInfo info = bmp::parseBmp(buf.data(), buf.size());
  CHECK(info.valid);
  CHECK_EQ(info.width, 8);
  CHECK_EQ(info.height, 2);
  CHECK_EQ((int)info.bitsPerPixel, 1);
  CHECK_EQ((int)info.rowSize, 4);
  CHECK_EQ((int)info.dataOffset, 62);
  CHECK(!info.topDown);
}

TEST(bmp_pixel_black_uses_palette) {
  auto buf = buildBmp(8, 2, {{0x00, 0, 0, 0}, {0xFF, 0, 0, 0}});
  bmp::BmpInfo info = bmp::parseBmp(buf.data(), buf.size());
  CHECK(info.valid);
  // Row 0 (top) is index 0 -> black.
  CHECK(bmp::pixelBlack(buf.data(), buf.size(), info, 0, 0));
  CHECK(bmp::pixelBlack(buf.data(), buf.size(), info, 7, 0));
  // Row 1 (bottom) is index 1 -> white.
  CHECK(!bmp::pixelBlack(buf.data(), buf.size(), info, 0, 1));
}

TEST(bmp_pixel_out_of_bounds_is_white) {
  auto buf = buildBmp(8, 2, {{0x00, 0, 0, 0}, {0xFF, 0, 0, 0}});
  bmp::BmpInfo info = bmp::parseBmp(buf.data(), buf.size());
  CHECK(!bmp::pixelBlack(buf.data(), buf.size(), info, -1, 0));
  CHECK(!bmp::pixelBlack(buf.data(), buf.size(), info, 8, 0));
  CHECK(!bmp::pixelBlack(buf.data(), buf.size(), info, 0, 2));
}

TEST(bmp_rejects_truncated_pixel_data) {
  auto buf = buildBmp(8, 2, {{0x00, 0, 0, 0}, {0xFF, 0, 0, 0}});
  buf.resize(buf.size() - 2);  // chop the last row short
  bmp::BmpInfo info = bmp::parseBmp(buf.data(), buf.size());
  CHECK(!info.valid);
}
