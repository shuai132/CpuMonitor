#include "CpuMonitor.h"

namespace cpu_monitor {

CpuMonitor::CpuMonitor() = default;

CpuStat &CpuMonitor::stat() {
  return cpuTickCurr();
}

void CpuMonitor::invertAB() {
  currentA_ = !currentA_;
}

CpuStat &CpuMonitor::cpuTickCurr() {
  return currentA_ ? cpuTickA_ : cpuTickB_;
}

CpuStat &CpuMonitor::cpuTickPre() {
  return !currentA_ ? cpuTickA_ : cpuTickB_;
}

void CpuMonitor::updateFromFile(FILE *fp) {
  auto &cpuTick = cpuTickCurr();
  // clang-format off
  fscanf(fp, // NOLINT
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
}

void CpuMonitor::calcUsage() {
  totalTime = cpuTickCurr().calcTicksTotal() - cpuTickPre().calcTicksTotal();
  idleTime = cpuTickCurr().calcTicksIdle() - cpuTickPre().calcTicksIdle();
  uint64_t active = totalTime - idleTime;
  usage = active * 100.f / totalTime;  // NOLINT
}

}  // namespace cpu_monitor
