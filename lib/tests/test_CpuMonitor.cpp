#include <unistd.h>

#include "CpuMonitor.h"
#include "assert_def.h"
#include "detail/log.h"

using namespace cpu_monitor;

int main() {
  CpuMonitor monitor;
  cpu_monitor_LOGI("cores num: %zu", monitor.cores.size());

  for (;;) {
    sleep(1);
    monitor.update(true);
    monitor.dump();
  }
  return 0;
}
