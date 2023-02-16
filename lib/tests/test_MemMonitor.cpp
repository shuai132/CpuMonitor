#include <unistd.h>

#include "MemMonitor.h"
#include "assert_def.h"
#include "detail/log.h"

using namespace cpu_monitor;

int main(int argc, char** argv) {
  cpu_monitor_LOGI("=> dumpUsage");
  MemMonitor::dumpUsage(getpid());

  cpu_monitor_LOGI("=> getUsage");
  auto ret = MemMonitor::getUsage(getpid());
  cpu_monitor_LOGI("ret: ok=%d", ret.ok);
  cpu_monitor_LOGI("VmPeak:%lu", ret.usage.VmPeak);
  cpu_monitor_LOGI("VmSize:%lu", ret.usage.VmSize);
  cpu_monitor_LOGI("VmHWM:%lu", ret.usage.VmHWM);
  cpu_monitor_LOGI("VmRSS:%lu", ret.usage.VmRSS);
  return 0;
}
