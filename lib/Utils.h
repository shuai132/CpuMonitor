#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Types.h"

namespace cpu_monitor {
namespace Utils {

struct TasksRet {
  bool ok = false;
  std::string name;
  std::vector<TaskId_t> ids;
};

TasksRet getTasksOfPid(PID_t pid);

PID_t getPidByName(const std::string& name);

}  // namespace Utils
}  // namespace cpu_monitor
