#include "Utils.h"

#include <dirent.h>

#include <cstring>
#include <fstream>
#include <string>

#include "TaskMonitor.h"
#include "detail/defer.h"

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
  ret.name = TaskMonitor(pid, nullptr).stat().name;

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

PID_t getFirstPidByName(const std::string& name) {
  DIR* dir = opendir("/proc");
  defer {
    closedir(dir);
  };
  if (dir == nullptr) {
    return 0;
  }

  struct dirent* ptr;
  while ((ptr = readdir(dir)) != nullptr) {
    if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) continue;
    if (DT_DIR != ptr->d_type) continue;

    const auto& pidName = ptr->d_name;

    std::string filePath = "/proc/" + std::string(pidName) + "/status";
    std::fstream file(filePath, std::fstream::in);
    if (!file.is_open()) {
      continue;
    }

    // Name: xxxx
    std::string tmpName;
    file >> tmpName;
    file >> tmpName;
    file.close();

    if (std::string(name).compare(0, tmpName.size(), tmpName) == 0) {
      return std::strtoul(pidName, nullptr, 10);
    }
  }
  return 0;
}

}  // namespace Utils
}  // namespace cpu_monitor
