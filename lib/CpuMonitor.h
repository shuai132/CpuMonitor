#pragma once

#include <memory>
#include <vector>

#include "CpuMonitorCore.h"
#include "detail/noncopyable.hpp"

namespace cpu_monitor {

class CpuMonitor : detail::noncopyable {
 public:
  explicit CpuMonitor();

  void update(bool updateCores = true);
  void dump() const;

 public:
  std::unique_ptr<CpuMonitorCore> ave;
  std::vector<std::unique_ptr<CpuMonitorCore>> cores;
};

}  // namespace cpu_monitor
