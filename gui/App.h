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
  void poll();

  void dispatch(const std::function<void()>& task);

  void post(std::function<void()> task);

 private:
  asio::io_context context_;
};
