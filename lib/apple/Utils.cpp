#include "Utils.h"

#include <libproc.h>
#include <mach/mach.h>
#include <unistd.h>

#include <string>

#include "detail/defer.h"

namespace cpu_monitor {
namespace Utils {

TasksRet getTasksOfPid(PID_t pid) {
  TasksRet ret;

  task_t task;
  kern_return_t kr = task_for_pid(mach_task_self(), (int)pid, &task);
  if (kr != KERN_SUCCESS) {
    ret.ok = false;
    return ret;
  }

  thread_act_array_t threads;
  mach_msg_type_number_t thread_count = 0;
  kr = task_threads(task, &threads, &thread_count);
  if (kr != KERN_SUCCESS) {
    ret.ok = false;
    return ret;
  }

  char name[1024];
  proc_name((int)pid, name, sizeof(name));
  ret.name = name;
  for (int i = 0; i < thread_count; ++i) {
    ret.ids.push_back(threads[i]);
  }

  ret.ok = true;
  return ret;
}

PID_t getPidByName(const std::string& name) {
  int precessNum = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
  pid_t pids[precessNum];
  proc_listpids(PROC_ALL_PIDS, 0, pids, precessNum);
  for (int i = 0; i < precessNum; ++i) {
    if (pids[i] == 0) {
      continue;
    }
    char name_tmp[1024];
    proc_name(pids[i], name_tmp, sizeof(name_tmp));
    if (name == name_tmp) {
      return pids[i];
    }
  }
  return 0;
}

}  // namespace Utils
}  // namespace cpu_monitor
