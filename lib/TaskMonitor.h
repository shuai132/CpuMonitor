#pragma once

#include <functional>
#include <string>

#include "CpuMonitor.h"
#include "Types.h"
#include "detail/noncopyable.hpp"

namespace cpu_monitor {

/**
 * Task means process or thread
 */
class TaskMonitor : detail::noncopyable {
  using TotalTimeImpl = std::function<uint64_t()>;

 public:
  explicit TaskMonitor(TaskId_t tid, TotalTimeImpl cpuTicksImpl);

  bool update();

  void dump() const;

 public:
  std::string name;
  TaskId_t id;
  float usage{};

 private:
  TotalTimeImpl totalTimeImpl_;
  uint64_t totalThreadTime_{};
};

}  // namespace cpu_monitor
