#pragma once

#include <cstdint>

#include "noncopyable.hpp"

class Home : noncopyable {
 public:
  Home();
  ~Home();

 public:
  static void onDraw();

 private:
  static void initGUI();
};
