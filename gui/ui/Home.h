#pragma once

#include <cstdint>

#include "noncopyable.hpp"

class Home : noncopyable {
 public:
  Home();
  ~Home();

 public:
  void onDraw() const;

 private:
  void initGUI();

 public:
  uint32_t windowFlags_ = 0;
};
