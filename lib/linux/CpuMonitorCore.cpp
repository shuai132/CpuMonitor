#include "CpuMonitorCore.h"

#include <stdexcept>

#include "CpuStat.h"
#include "detail/log.h"

namespace cpu_monitor {

CpuMonitorCore::CpuMonitorCore() = default;

void CpuMonitorCore::invertAB() {
  currentA_ = !currentA_;
}

CpuTick &CpuMonitorCore::tickCur() {
  return currentA_ ? cpuTickA_ : cpuTickB_;
}

CpuTick &CpuMonitorCore::tickPre() {
  return !currentA_ ? cpuTickA_ : cpuTickB_;
}

void CpuMonitorCore::update(CpuInfoNative *p) {
  invertAB();
  {
    using namespace detail;
    detail::CpuStat cpuTick;  // NOLINT
    // clang-format off
    int ret = fscanf(p, // NOLINT
          "%s"
          " %" PRIu64
          " %" PRIu64
          " %" PRIu64
          " %" PRIu64
          " %" PRIu64
          " %" PRIu64
          " %" PRIu64
          " %" PRIu64
          " %" PRIu64
          " %" PRIu64
          "\n",
          cpuTick.name,
          &(cpuTick.ticks[CpuT::USER]),
          &(cpuTick.ticks[CpuT::NICE]),
          &(cpuTick.ticks[CpuT::SYSTEM]),
          &(cpuTick.ticks[CpuT::IDLE]),
          &(cpuTick.ticks[CpuT::IOWAIT]),
          &(cpuTick.ticks[CpuT::IRQ]),
          &(cpuTick.ticks[CpuT::SOFTIRQ]),
          &(cpuTick.ticks[CpuT::STEAL]),
          &(cpuTick.ticks[CpuT::GUEST]),
          &(cpuTick.ticks[CpuT::GUEST_NICE]));
    // clang-format on
    if (!ret) throw std::runtime_error("CpuMonitorCore::update failed");

    name = cpuTick.name;

    auto &t = tickCur();
    t.idleTicks = cpuTick.calcTicksIdle();
    t.totalTicks = cpuTick.calcTicksTotal();
  }
  calcUsage();
}

void CpuMonitorCore::calcUsage() {
  totalTime = tickCur().totalTicks - tickPre().totalTicks;
  idleTime = tickCur().idleTicks - tickPre().idleTicks;
  uint64_t active = totalTime - idleTime;
  usage = active * 100.f / totalTime;  // NOLINT
}

void CpuMonitorCore::dump() const {
  cpu_monitor_LOGI("%s: usage: %.2f%%", name.c_str(), usage);
}

}  // namespace cpu_monitor
