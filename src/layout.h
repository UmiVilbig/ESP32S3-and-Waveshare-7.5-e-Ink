// layout.h — pure, hardware-independent text/geometry helpers.
//
// This module contains NO Arduino or hardware dependencies so it can be
// compiled and unit-tested natively on a host machine (see /test) as well as
// on the ESP32-S3.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace layout {

// Greatest common divisor (Euclid). gcd(0, 0) == 0.
int gcd(int a, int b);

// Reduce a WxH resolution to its simplest aspect ratio string, e.g.
// aspectRatio(800, 480) == "5:3". Returns "0:0" if either side is <= 0.
std::string aspectRatio(int width, int height);

// Greedy word-wrap `text` into lines no longer than `maxChars` characters.
// Words longer than maxChars are hard-split. Runs of whitespace collapse to a
// single space. Returns an empty vector for empty/blank input. If maxChars <= 0
// the whole (trimmed) text is returned as a single line.
std::vector<std::string> wrapText(const std::string& text, int maxChars);

}  // namespace layout
