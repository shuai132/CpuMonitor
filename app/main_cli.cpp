#include <unistd.h>

#include <algorithm>
#include <memory>
#include <set>

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
  CpuMonitorAll cpu;
  const auto totalTimeImpl = [&cpu] {
    return cpu.ave->totalTime;
  };
  std::set<std::unique_ptr<TaskMonitor>> tasks;

  // init monitor
  {
    auto ret = Utils::getTasksOfPid(pid);
    LOGI("get task info: pid: %u, ok: %d, num: %zu", pid, ret.ok, ret.ids.size());
    if (!ret.ok) {
      LOGE("pid not found: %u", pid);
      return;
    }

    for (auto tid : ret.ids) {
      LOGI("thread id: %d", tid);
      tasks.insert(std::make_unique<TaskMonitor>(pid, tid, totalTimeImpl));
    }
  }

  for (;;) {
    sleep(s_updateIntervalSec);
    cpu.update(false);
    printf("system %s usage: %.2f%%\n", cpu.ave->stat().name, cpu.ave->usage);

    for (auto& item : tasks) {
      bool ok = item->update();
      if (ok) {
        printf("name: %-15s, id: %-7" PRIu64 ", usage: %.2f%%\n", item->stat().name.c_str(), item->stat().id, item->usage);
      } else {
        printf("thread exit: name: %s, id: %" PRIu64 "\n", item->stat().name.c_str(), item->stat().id);
      }
    }
    printf("\n");

    const auto& taskRet = Utils::getTasksOfPid(pid);
    if (!taskRet.ok) {
      printf("progress exit! will wait...\n");
      return;
    }
    const auto& tasksNow = taskRet.ids;

    // 删除已不存在的线程
    const auto isTaskAlive = [&tasksNow](TaskId_t taskId) {
      return std::find(tasksNow.cbegin(), tasksNow.cend(), taskId) != tasksNow.cend();
    };
    for (auto iter = tasks.begin(); iter != tasks.cend();) {
      // delete not cared task
      if (!isTaskAlive(iter->get()->stat().id)) {
        iter = tasks.erase(iter);
      } else {
        ++iter;
      }
    }

    // 添加新增的线程
    const auto isNewTask = [&tasks](TaskId_t taskId) {
      return std::find_if(tasks.cbegin(), tasks.cend(), [&](const std::unique_ptr<TaskMonitor>& monitor) {
               return monitor->stat().id == taskId;
             }) == tasks.cend();
    };
    for (const auto& item : tasksNow) {
      if (isNewTask(item)) {
        tasks.insert(std::make_unique<TaskMonitor>(pid, item, totalTimeImpl));
      }
    }
  }
}

static void monitorProgram(const std::string& name) {
  for (;;) {
    auto pid = Utils::getFirstPidByName(name);
    if (pid == 0) {
      LOGE("pid not found by name: %s", name.c_str());
    } else {
      monitorPid(pid);
    }
    sleep(1);
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
  while ((ret = getopt(argc, argv, "s:c::p:n:")) != -1) {
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
      case 'n': {
        auto& name = optarg;
        LOGD("program name: %s", name);
        monitorProgram(name);
      } break;
      default:
        showHelp();
        break;
    }
  }
}
