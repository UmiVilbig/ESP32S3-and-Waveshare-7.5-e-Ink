#include "layout.h"

#include <cctype>
#include <sstream>

namespace layout {

int gcd(int a, int b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b != 0) {
    int t = a % b;
    a = b;
    b = t;
  }
  return a;
}

std::string aspectRatio(int width, int height) {
  if (width <= 0 || height <= 0) return "0:0";
  int g = gcd(width, height);
  return std::to_string(width / g) + ":" + std::to_string(height / g);
}

std::vector<std::string> wrapText(const std::string& text, int maxChars) {
  // Tokenize on any whitespace, collapsing runs.
  std::vector<std::string> words;
  {
    std::string cur;
    for (char c : text) {
      if (std::isspace(static_cast<unsigned char>(c))) {
        if (!cur.empty()) {
          words.push_back(cur);
          cur.clear();
        }
      } else {
        cur.push_back(c);
      }
    }
    if (!cur.empty()) words.push_back(cur);
  }

  std::vector<std::string> lines;
  if (words.empty()) return lines;

  if (maxChars <= 0) {
    // Join everything into one line.
    std::string one = words[0];
    for (size_t i = 1; i < words.size(); ++i) one += " " + words[i];
    lines.push_back(one);
    return lines;
  }

  std::string line;
  for (const std::string& word : words) {
    // Hard-split any word that cannot fit on a line by itself.
    if (static_cast<int>(word.size()) > maxChars) {
      if (!line.empty()) {
        lines.push_back(line);
        line.clear();
      }
      size_t pos = 0;
      while (pos < word.size()) {
        lines.push_back(word.substr(pos, maxChars));
        pos += maxChars;
      }
      // Last chunk may be partial and can accept more words; keep it open.
      if (!lines.empty() &&
          static_cast<int>(lines.back().size()) < maxChars) {
        line = lines.back();
        lines.pop_back();
      }
      continue;
    }

    if (line.empty()) {
      line = word;
    } else if (static_cast<int>(line.size() + 1 + word.size()) <= maxChars) {
      line += " " + word;
    } else {
      lines.push_back(line);
      line = word;
    }
  }
  if (!line.empty()) lines.push_back(line);
  return lines;
}

}  // namespace layout
