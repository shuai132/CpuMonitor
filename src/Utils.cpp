#include "Utils.h"

#include <dirent.h>

#include <string>

namespace cpu_monitor {
namespace Utils {

TasksRet getTasksOfPid(PID_t pid) {
  TasksRet ret;

  std::string taskPath = "/proc/" + std::to_string(pid) + "/task";
  auto dir = opendir(taskPath.c_str());
  if (dir == nullptr) {
    ret.ok = false;
    return ret;
  }

  ret.ok = true;
  while (true) {
    auto p = readdir(dir);
    if (p == nullptr) break;

    std::string dirName(p->d_name);
    if (dirName == "." || dirName == "..") continue;

    auto taskId = std::strtoul(p->d_name, nullptr, 10);
    ret.ids.push_back(taskId);
  }
  closedir(dir);

  return ret;
}

std::string makeTaskPath(PID_t pid, TaskId_t tid) {
  std::string path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid);
  return path;
}

std::string makeTaskStatPath(PID_t pid, TaskId_t tid) {
  std::string path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid) + "/stat";
  return path;
}

}  // namespace Utils
}  // namespace cpu_monitor
