#include "CpuMonitor.h"

#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/processor_info.h>
#include <mach/semaphore.h>

#include <stdexcept>
#include <thread>

#include "detail/defer.h"
#include "detail/log.h"

namespace cpu_monitor {

CpuMonitor::CpuMonitor() {
  ave = std::make_unique<CpuMonitorCore>();
  ave->name = "cpu";

  auto cpuNum = std::thread::hardware_concurrency();

  cpu_monitor_LOGI("cpu core: %d", cpuNum);
  for (int i = 0; i < cpuNum; i++) {
    auto monitor = std::make_unique<CpuMonitorCore>();
    monitor->name = "cpu" + std::to_string(i);
    cores.push_back(std::move(monitor));
  }
  update();
}

void CpuMonitor::update(bool updateCores) {
  // update ave
  {
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    host_cpu_load_info_data_t load;
    auto kr = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&load, &count);
    if (kr != KERN_SUCCESS) throw std::runtime_error("host_statistics failed");

    ave->update(load.cpu_ticks);
  }
  if (!updateCores) return;

  // update cores
  {
    natural_t cpuNum;
    processor_info_array_t cpuInfo;
    mach_msg_type_number_t cpuInfoNum;
    auto kr = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuNum, &cpuInfo, &cpuInfoNum);
    if (kr != KERN_SUCCESS) throw std::runtime_error("host_processor_info failed");

    for (const auto &item : cores) {
      item->update((CpuInfoNative *)cpuInfo);
      cpuInfo += CPU_STATE_MAX;
    }
  }
}

void CpuMonitor::dump() const {
  ave->dump();
  for (const auto &item : cores) {
    item->dump();
  }
}

}  // namespace cpu_monitor
