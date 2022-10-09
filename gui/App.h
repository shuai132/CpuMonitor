#pragma once

#include "asio.hpp"
#include "noncopyable.hpp"

class App : noncopyable {
 public:
  using Runnable = std::function<void()>;
  static App* instance();

 private:
  App();

 public:
  void onDraw();

  void dispatch(const std::function<void()>& task);

  void post(std::function<void()> task);

 private:
  void initGUI();

 public:
  static const int MainWindowWidth = 1280;
  static const int MainWindowHeight = 720;
  static const char* MainWindowTitle;

 private:
  asio::io_context context_;

  uint32_t windowFlags_ = 0;
};
