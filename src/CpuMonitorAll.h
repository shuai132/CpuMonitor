#pragma once

#include <memory>
#include <vector>

#include "CpuMonitor.h"

namespace cpu_monitor {

class CpuMonitorAll : noncopyable {
 public:
  explicit CpuMonitorAll();

  void update(bool updateCores = true);
  void dump();

 public:
  std::unique_ptr<CpuMonitor> ave;
  std::vector<std::unique_ptr<CpuMonitor>> cores;
};

}  // namespace cpu_monitor
