#include "../src/layout.h"
#include "test_framework.h"

using layout::aspectRatio;
using layout::gcd;
using layout::wrapText;
using Lines = std::vector<std::string>;

TEST(gcd_basic) {
  CHECK_EQ(gcd(800, 480), 160);
  CHECK_EQ(gcd(1920, 1080), 120);
  CHECK_EQ(gcd(7, 13), 1);
  CHECK_EQ(gcd(0, 5), 5);
  CHECK_EQ(gcd(5, 0), 5);
  CHECK_EQ(gcd(-12, 8), 4);
}

TEST(aspect_ratio_common) {
  CHECK_EQ(aspectRatio(800, 480), std::string("5:3"));
  CHECK_EQ(aspectRatio(1920, 1080), std::string("16:9"));
  CHECK_EQ(aspectRatio(1024, 768), std::string("4:3"));
  CHECK_EQ(aspectRatio(500, 500), std::string("1:1"));
}

TEST(aspect_ratio_invalid) {
  CHECK_EQ(aspectRatio(0, 480), std::string("0:0"));
  CHECK_EQ(aspectRatio(800, 0), std::string("0:0"));
  CHECK_EQ(aspectRatio(-1, 5), std::string("0:0"));
}

TEST(wrap_fits_on_one_line) {
  CHECK_EQ(wrapText("hello world", 20), (Lines{"hello world"}));
}

TEST(wrap_breaks_on_spaces) {
  CHECK_EQ(wrapText("hello world", 5), (Lines{"hello", "world"}));
  CHECK_EQ(wrapText("a b c", 1), (Lines{"a", "b", "c"}));
}

TEST(wrap_hard_splits_long_word) {
  CHECK_EQ(wrapText("aaaaaaaa", 3), (Lines{"aaa", "aaa", "aa"}));
}

TEST(wrap_long_word_then_word) {
  CHECK_EQ(wrapText("aaaaaa bb", 3), (Lines{"aaa", "aaa", "bb"}));
}

TEST(wrap_collapses_whitespace) {
  CHECK_EQ(wrapText("a   b", 10), (Lines{"a b"}));
  CHECK_EQ(wrapText("  hi  there  ", 20), (Lines{"hi there"}));
}

TEST(wrap_empty_and_blank) {
  CHECK_EQ(wrapText("", 10), (Lines{}));
  CHECK_EQ(wrapText("    ", 10), (Lines{}));
}

TEST(wrap_nonpositive_width_is_single_line) {
  CHECK_EQ(wrapText("a b c", 0), (Lines{"a b c"}));
  CHECK_EQ(wrapText("a b c", -5), (Lines{"a b c"}));
}
