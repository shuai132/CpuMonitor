#pragma once

#include <cinttypes>
#include <cstdint>
#include <string>

#include "CpuInfoNative.h"
#include "CpuTick.h"
#include "detail/noncopyable.hpp"

namespace cpu_monitor {

class CpuMonitorCore : detail::noncopyable {
 public:
  explicit CpuMonitorCore();

 public:
  std::string name;
  float usage{};

  uint64_t idleTime{};
  uint64_t totalTime{};

  void update(CpuInfoNative *p);
  void dump() const;

 private:
  void invertAB();
  CpuTick &tickCur();
  CpuTick &tickPre();
  void calcUsage();

 private:
  bool currentA_{};
  CpuTick cpuTickA_{};
  CpuTick cpuTickB_{};
};

}  // namespace cpu_monitor
