#include "CpuMonitor.h"

#include <stdexcept>

#include "detail/defer.h"
#include "detail/log.h"
#include "sys/sysinfo.h"

namespace cpu_monitor {

CpuMonitor::CpuMonitor() {
  ave = std::make_unique<CpuMonitorCore>();

  int cpuNum = get_nprocs();
  cpu_monitor_LOGI("cpuNum: %d", cpuNum);
  for (int i = 0; i < cpuNum; i++) {
    auto monitor = std::make_unique<CpuMonitorCore>();
    cores.push_back(std::move(monitor));
  }

  update();
}

void CpuMonitor::update(bool updateCores) {
  FILE *fp = fopen("/proc/stat", "r");
  if (fp == nullptr) {
    cpu_monitor_LOGE("open failed: /proc/stat");
    return;
  }
  defer {
    fclose(fp);
  };

  ave->update(fp);

  if (!updateCores) return;

  for (const auto &item : cores) {
    item->update(fp);
  }
}

void CpuMonitor::dump() const {
  ave->dump();
  for (const auto &item : cores) {
    item->dump();
  }
}

}  // namespace cpu_monitor
