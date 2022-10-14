#pragma once

#include <cinttypes>
#include <cstdint>
#include <string>

#include "CpuStat.h"
#include "detail/noncopyable.hpp"

namespace cpu_monitor {

class CpuMonitor : noncopyable {
 public:
  explicit CpuMonitor();

 public:
  CpuStat &stat();

 public:
  float usage{};
  uint64_t idleTime{};
  uint64_t totalTime{};

  friend class CpuMonitorAll;

 private:
  void updateFromFile(FILE *file);
  void calcUsage();

 private:
  void invertAB();
  CpuStat &cpuTickCurr();
  CpuStat &cpuTickPre();

 private:
  bool currentA_{};
  CpuStat cpuTickA_{};
  CpuStat cpuTickB_{};
};

}  // namespace cpu_monitor
