#pragma once

#include <cstdint>

namespace cpu_monitor {

struct CpuTick {
  uint32_t idleTicks = 0;
  uint32_t totalTicks = 0;
};

}  // namespace cpu_monitor
