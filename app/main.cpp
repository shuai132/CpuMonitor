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
static std::vector<PID_t> s_monitorPids;
static std::vector<std::string> s_monitorNames{"node"};


static void sendMessage() {

}


int main(int argc, char** argv) {
  auto pid = Utils::getFirstPidByName(s_monitorNames.front());

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
      return 0;
    }

    for (auto tid : ret.ids) {
      LOGI("thread id: %d", tid);
      tasks.insert(std::make_unique<TaskMonitor>(Utils::makeTaskStatPath(pid, tid), totalTimeImpl));
    }
  }

  for (;;) {
    sleep(s_updateIntervalSec);
    cpu.update(true);
    printf("system %s usage: %.2f%%\n", cpu.ave->stat().name, cpu.ave->usage);

    for (auto& item : tasks) {
      bool ok = item->update();
      if (ok) {
        printf("name: %s, id: %lu, usage: %.2f%%\n", item->stat().name.c_str(), item->stat().id, item->usage * 100);
      } else {
        printf("thread exit: name: %s, id: %lu\n", item->stat().name.c_str(), item->stat().id);
      }
    }
    printf("\n");

    const auto& taskRet = Utils::getTasksOfPid(pid);
    if (!taskRet.ok) {
      printf("progress exit! will wait...\n");
      return 0;
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
        tasks.insert(std::make_unique<TaskMonitor>(Utils::makeTaskStatPath(pid, item), totalTimeImpl));
      }
    }
  }
}
