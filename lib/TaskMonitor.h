#pragma once

#include <functional>
#include <string>

#include "CpuMonitor.h"
#include "Types.h"
#include "detail/noncopyable.hpp"
#include "linux/TaskStat.h"

namespace cpu_monitor {

/**
 * Task means process or thread
 */
class TaskMonitor : detail::noncopyable {
  using TotalTimeImpl = std::function<uint64_t()>;

 public:
  explicit TaskMonitor(TaskId_t tid, TotalTimeImpl totalTimeImpl);

  bool update();

  void dump();

 public:
  std::string name;
  TaskId_t id;
  float usage{};

  uint64_t totalThreadTime{};
  uint64_t totalCpuTime{};

#ifdef __linux__
  using TaskStat = detail::TaskStat;

 private:
  void invertAB();

  TaskStat &taskStatCurr();

  TaskStat &taskStatPre();

  bool updateStat();

  void calcUsage();

 private:
  bool currentA_{};
  TaskStat cpuTickA_{};
  TaskStat cpuTickB_{};
#endif

 private:
  TotalTimeImpl totalTimeImpl_;
};

}  // namespace cpu_monitor
