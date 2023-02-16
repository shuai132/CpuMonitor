#include "MemMonitor.h"

#include <libproc.h>
#include <sys/proc_info.h>

#include <cinttypes>
#include <cstdio>

namespace cpu_monitor {

MemMonitor::UsageRet MemMonitor::getUsage(PID_t pid) {
  if (pid == 0) pid = getpid();

  UsageRet usageRet;  // NOLINT
  auto &result = usageRet.usage;

  proc_taskinfo info;  // NOLINT
  proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info));

  result.VmPeak = 0;
  result.VmSize = info.pti_virtual_size / 1024;
  result.VmHWM = 0;
  result.VmRSS = info.pti_resident_size / 1024;
  usageRet.ok = true;
  return usageRet;
}

void MemMonitor::dumpUsage(PID_t pid) {
  if (pid == 0) pid = getpid();
  proc_taskinfo info;  // NOLINT
  proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info));
  printf("virtual memory size (bytes): %" PRIu64 "\n", info.pti_virtual_size);
  printf("resident memory size (bytes): %" PRIu64 "\n", info.pti_resident_size);
}

}  // namespace cpu_monitor
