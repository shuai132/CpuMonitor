#include <unistd.h>

#include "CpuMonitor.h"
#include "TaskMonitor.h"
#include "Utils.h"
#include "assert_def.h"
#include "detail/log.h"

using namespace cpu_monitor;

int main() {
  auto id = Utils::getTasksOfPid(getpid()).ids.front();
  cpu_monitor_LOGI("task id: %d", id);

  CpuMonitor monitorAll;
  TaskMonitor monitor(id, [&] {
    return monitorAll.ave->totalTime;
  });

  for (;;) {
    sleep(1);
    monitorAll.update();
    bool ret = monitor.update();
    ASSERT(ret);
    monitor.dump();
  }
  return 0;
}
