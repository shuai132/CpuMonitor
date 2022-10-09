#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Types.h"

namespace cpu_monitor {
namespace Utils {

struct TasksRet {
  bool ok = false;
  std::vector<TaskId_t> ids;
};

TasksRet getTasksOfPid(PID_t pid);

std::string makeTaskPath(PID_t pid, TaskId_t tid);
std::string makeTaskStatPath(PID_t pid, TaskId_t tid);

}  // namespace Utils
}  // namespace cpu_monitor
