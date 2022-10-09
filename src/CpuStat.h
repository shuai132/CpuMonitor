#pragma once

#include <cstdint>

namespace cpu_monitor {

namespace CpuT {
enum : int {
  USER = 0,
  NICE,
  SYSTEM,
  IDLE,
  IOWAIT,
  IRQ,
  SOFTIRQ,
  STEAL,
  GUEST,
  GUEST_NICE,
  TYPE_NUM,
};
}

struct CpuStat {
  char name[16];
  uint64_t ticks[CpuT::TYPE_NUM];

  uint64_t calcTicksIdle() {
    return ticks[CpuT::IDLE] + ticks[CpuT::IOWAIT];
  }

  uint64_t calcTicksTotal() {
    uint64_t total = 0;
    for (auto tick : ticks) total += tick;
    return total;
  }
};

}  // namespace cpu_monitor
