#include <unistd.h>

#include "TaskMonitor.h"
#include "Utils.h"
#include "assert_def.h"
#include "detail/log.h"

using namespace cpu_monitor;

int main(int argc, char** argv) {
  cpu_monitor_LOGI("=> getTasksOfPid");
  {
    auto ret = Utils::getTasksOfPid(getpid());
    cpu_monitor_LOGI("process name: %s", ret.name.c_str());
    for (const auto& item : ret.ids) {
      cpu_monitor_LOGI("task id: %d", item);
    }
  }

  cpu_monitor_LOGI("=> getPidByName");
  {
    std::string name(argv[0]);
    name = name.substr(name.rfind('/') + 1);
    cpu_monitor_LOGI("process name: %s", name.c_str());
    auto pid = Utils::getPidByName(name);
    cpu_monitor_LOGI("process pid: %d", pid);
  }

  return 0;
}
