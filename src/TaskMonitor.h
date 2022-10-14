#pragma once

#include <functional>
#include <string>

#include "CpuMonitor.h"
#include "TaskStat.h"
#include "Types.h"
#include "detail/noncopyable.hpp"

namespace cpu_monitor {

/**
 * process and thread
 */
class TaskMonitor : noncopyable {
  using TotalTimeImpl = std::function<uint64_t()>;

 public:
  explicit TaskMonitor(std::string path, TotalTimeImpl totalTimeImpl);
  explicit TaskMonitor(PID_t pid, TaskId_t tid, TotalTimeImpl totalTimeImpl);
  explicit TaskMonitor(PID_t pid, TotalTimeImpl totalTimeImpl);

  bool update();

  void dump();

  TaskStat &stat();

 private:
  void invertAB();

  TaskStat &taskStatCurr();

  TaskStat &taskStatPre();

  bool updateFromStat();

  void calcUsage();

 public:
  float usage{};
  uint64_t totalThreadTime{};
  uint64_t totalCpuTime{};

 private:
  bool currentA_{};
  TaskStat cpuTickA_{};
  TaskStat cpuTickB_{};

 private:
  TotalTimeImpl totalTimeImpl_;
  std::string statPath_;
};

}  // namespace cpu_monitor
