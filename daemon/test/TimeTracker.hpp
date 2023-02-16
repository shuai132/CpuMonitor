#pragma once

#include <chrono>
#include <cstdint>

class TimeTracker {
 public:
  TimeTracker() {
    reset();
  }

  TimeTracker(const TimeTracker&) = delete;
  void operator=(const TimeTracker&) = delete;
  TimeTracker(TimeTracker&&) = default;

  void reset() {
    start_ = std::chrono::steady_clock::now();
  }

  template <typename T>
  uint64_t elapsed() {
    auto now = std::chrono::steady_clock::now();
    auto ret = std::chrono::duration_cast<T>(now - start_).count();
    return ret;
  }

  uint64_t ms() {
    return elapsed<std::chrono::milliseconds>();
  }

  uint64_t us() {
    return elapsed<std::chrono::microseconds>();
  }

  uint64_t sec() {
    return elapsed<std::chrono::seconds>();
  }

 private:
  std::chrono::time_point<std::chrono::steady_clock> start_;
};
