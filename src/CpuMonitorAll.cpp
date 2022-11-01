#include "CpuMonitorAll.h"

#include "CpuMonitor.h"
#include "detail/defer.h"
#include "detail/log.h"
#include "sys/sysinfo.h"

namespace cpu_monitor {

CpuMonitorAll::CpuMonitorAll() {
  ave = std::make_unique<CpuMonitor>();

  int cpuNum = get_nprocs();
  cpu_monitor_LOGI("cpuNum: %d", cpuNum);
  for (int i = 0; i < cpuNum; i++) {
    auto monitor = std::make_unique<CpuMonitor>();
    cores.push_back(std::move(monitor));
  }
  update();
}

void CpuMonitorAll::update(bool updateCores) {
  FILE *fp = fopen("/proc/stat", "r");
  defer {
    if (fp == nullptr) return;
    fclose(fp);
  };
  if (fp == nullptr) throw std::runtime_error("can not open /proc/stat");

  ave->invertAB();
  ave->updateFromFile(fp);
  ave->calcUsage();

  if (!updateCores) return;

  for (const auto &item : cores) {
    item->invertAB();
    item->updateFromFile(fp);
    item->calcUsage();
  }
}

void CpuMonitorAll::dump() {
  cpu_monitor_LOGI("%s: usage: %.2f%%", ave->cpuTickCurr().name, ave->usage);
  for (const auto &item : cores) {
    cpu_monitor_LOGI("%s: usage: %.2f%%", item->cpuTickCurr().name, item->usage);
  }
}

}  // namespace cpu_monitor
