#pragma once

#include <cstdint>

namespace cpu_monitor {
namespace detail {

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

  uint64_t calcTicksIdle() {
    return ticks[CpuT::IDLE] + ticks[CpuT::IOWAIT];
  }

  uint64_t calcTicksTotal() {
    uint64_t total = 0;
    for (auto tick : ticks) total += tick;
    return total;
  }

  uint64_t calcTicksSystem() {
    return ticks[CpuT::SYSTEM];
  }

  uint64_t calcTicksUser() {
    return ticks[CpuT::USER];
  }

  uint64_t ticks[CpuT::TYPE_NUM];
};

}  // namespace detail
}  // namespace cpu_monitor
