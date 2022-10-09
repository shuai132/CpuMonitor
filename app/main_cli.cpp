#include <unistd.h>

#include <memory>

#include "CpuMonitor.h"
#include "CpuMonitorAll.h"
#include "TaskMonitor.h"
#include "Utils.h"
#include "log.h"

using namespace cpu_monitor;

static int s_updateIntervalSec = 1;

static void monitorCpu() {
  CpuMonitorAll cpu;
  for (;;) {
    sleep(s_updateIntervalSec);
    cpu.update();
    printf("%s usage: %.2f%%\n", cpu.ave->stat().name, cpu.ave->usage);
    for (auto& item : cpu.cores) {
      printf("%s usage: %.2f%%\n", item->stat().name, item->usage);
    }
    printf("\n");
  }
}

static void monitorPid(PID_t pid) {
  auto ret = Utils::getTasksOfPid(pid);
  LOGI("get task info: pid: %u, ok: %d, num: %zu", pid, ret.ok, ret.ids.size());
  if (!ret.ok) {
    LOGE("pid not found: %u", pid);
    return;
  }

  CpuMonitorAll cpu;
  std::vector<std::unique_ptr<TaskMonitor>> tasks;
  for (auto tid : ret.ids) {
    LOGI("thread id: %d", tid);
    tasks.push_back(std::make_unique<TaskMonitor>(Utils::makeTaskStatPath(pid, tid), [&cpu] {
      return cpu.ave->totalTime;
    }));
  }

  for (;;) {
    sleep(s_updateIntervalSec);
    cpu.update(false);
    printf("system %s usage: %.2f%%\n", cpu.ave->stat().name, cpu.ave->usage);

    for (auto& item : tasks) {
      bool ok = item->update();
      if (!ok) {
        LOGE("thread may exit");
      }
      printf("name: %s, id: %lu, usage: %.2f%%\n", item->stat().name.c_str(), item->stat().id, item->usage * 100);
    }
    printf("\n");
  }
}

static void showHelp() {
  printf(R"(Usage:
-s : 刷新间隔(s) 默认1
-c : 监控CPU
-t : 指定监控的PID
-n : 根据进程名称 查找到PID并进入监控
)");
}

int main(int argc, char** argv) {
  if (argc < 2) {
    showHelp();
    return 0;
  }

  int ret;
  while ((ret = getopt(argc, argv, "s:c::p:")) != -1) {
    switch (ret) {
      case 's': {
        s_updateIntervalSec = std::stoi(optarg, nullptr, 10);
        LOGD("s_updateIntervalSec: %d", s_updateIntervalSec);
      } break;
      case 'c':
        monitorCpu();
        break;
      case 'p': {
        auto pid = std::strtoul(optarg, nullptr, 10);
        monitorPid(pid);
      } break;
      default:
        showHelp();
        break;
    }
  }
}
