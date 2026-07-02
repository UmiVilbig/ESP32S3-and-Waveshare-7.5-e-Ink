// test_framework.h — a ~60-line, dependency-free unit test harness.
//
// Register tests with TEST(name){...}; assert with CHECK / CHECK_EQ. main.cpp
// calls tf::run(). No external libraries so it builds anywhere g++/clang lives.
#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace tf {

struct TestCase {
  std::string name;
  std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> r;
  return r;
}
inline int& failures() {
  static int f = 0;
  return f;
}
inline int& checks() {
  static int c = 0;
  return c;
}

struct Registrar {
  Registrar(const std::string& name, std::function<void()> fn) {
    registry().push_back({name, fn});
  }
};

// Pretty-printers for assertion failure messages.
template <class T>
std::string show(const T& v) {
  std::ostringstream os;
  os << v;
  return os.str();
}
inline std::string show(const std::string& v) { return "\"" + v + "\""; }
inline std::string show(bool v) { return v ? "true" : "false"; }
inline std::string show(const std::vector<std::string>& v) {
  std::string s = "[";
  for (size_t i = 0; i < v.size(); ++i) s += (i ? ", " : "") + show(v[i]);
  return s + "]";
}

inline int run() {
  int passed = 0;
  for (auto& tc : registry()) {
    int before = failures();
    try {
      tc.fn();
    } catch (const std::exception& e) {
      failures()++;
      std::cout << "  THREW " << tc.name << ": " << e.what() << "\n";
    }
    if (failures() == before) {
      ++passed;
    } else {
      std::cout << "FAILED: " << tc.name << "\n";
    }
  }
  std::cout << "\n" << passed << "/" << registry().size() << " tests passed, "
            << checks() << " checks, " << failures() << " failures\n";
  return failures() == 0 ? 0 : 1;
}

}  // namespace tf

#define TEST(name)                                       \
  static void name();                                    \
  static tf::Registrar reg_##name(#name, name);          \
  static void name()

#define CHECK(cond)                                                        \
  do {                                                                     \
    tf::checks()++;                                                        \
    if (!(cond)) {                                                         \
      tf::failures()++;                                                    \
      std::cout << "  FAIL " << __FILE__ << ":" << __LINE__ << ": CHECK(" \
                << #cond << ")\n";                                         \
    }                                                                      \
  } while (0)

#define CHECK_EQ(a, b)                                                       \
  do {                                                                      \
    tf::checks()++;                                                         \
    auto _va = (a);                                                        \
    auto _vb = (b);                                                        \
    if (!(_va == _vb)) {                                                   \
      tf::failures()++;                                                    \
      std::cout << "  FAIL " << __FILE__ << ":" << __LINE__ << ": CHECK_EQ(" \
                << #a << ", " << #b << ")\n    lhs=" << tf::show(_va)      \
                << "\n    rhs=" << tf::show(_vb) << "\n";                  \
    }                                                                       \
  } while (0)
