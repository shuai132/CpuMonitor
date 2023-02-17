#include "CpuMonitorCore.h"

#include <mach/processor_info.h>

#include "detail/log.h"

namespace cpu_monitor {

CpuMonitorCore::CpuMonitorCore() = default;

void CpuMonitorCore::update(CpuInfoNative *p) {
  invertAB();
  {
    auto &cpuTick = tickCur();
    auto cpu_ticks = p;
    cpuTick.idleTicks = cpu_ticks[CPU_STATE_IDLE];
    cpuTick.totalTicks = 0;
    for (int i = 0; i < CPU_STATE_MAX; ++i) {
      cpuTick.totalTicks += cpu_ticks[i];
    }
  }
  calcUsage();
}

void CpuMonitorCore::dump() const {
  cpu_monitor_LOGI("%s: usage: %.2f%%", name.c_str(), usage);
}

void CpuMonitorCore::invertAB() {
  currentA_ = !currentA_;
}

CpuTick &CpuMonitorCore::tickCur() {
  return currentA_ ? cpuTickA_ : cpuTickB_;
}

CpuTick &CpuMonitorCore::tickPre() {
  return !currentA_ ? cpuTickA_ : cpuTickB_;
}

void CpuMonitorCore::calcUsage() {
  totalTime = tickCur().totalTicks - tickPre().totalTicks;
  idleTime = tickCur().idleTicks - tickPre().idleTicks;
  uint64_t active = totalTime - idleTime;
  usage = active * 100.f / totalTime;  // NOLINT
}

}  // namespace cpu_monitor
