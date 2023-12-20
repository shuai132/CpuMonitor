#include "TaskMonitor.h"

#include <mach/mach_error.h>
#include <mach/thread_act.h>
#include <mach/thread_info.h>
#include <sys/types.h>

#include <iostream>

#include "Utils.h"
#include "detail/log.h"

namespace cpu_monitor {

TaskMonitor::TaskMonitor(TaskId_t tid, TaskMonitor::TotalTimeImpl cpuTicksImpl) : id(tid), totalTimeImpl_(std::move(cpuTicksImpl)) {
  (void)totalThreadTime_;
}

bool TaskMonitor::update() {
  mach_msg_type_number_t count = THREAD_INFO_MAX;
  thread_basic_info_data_t info;
  thread_act_t thread = id;
  auto kr = thread_info(thread, THREAD_BASIC_INFO, (thread_info_t)&info, &count);
  if (kr == KERN_SUCCESS) {
    double cpu = info.cpu_usage;
    pthread_t pt = pthread_from_mach_thread_np(thread);
    char name_tmp[256] = {0};
    if (pt) {
      pthread_getname_np(pt, name_tmp, sizeof(name_tmp));
      if (strlen(name_tmp) == 0) {
        snprintf(name_tmp, sizeof(name_tmp), "%d", id);
      }
      name = name_tmp;
    }
    usage = (float)(cpu / TH_USAGE_SCALE * 100);
    if (usage > 100) {  // invalid data
      usage = 0;
    }
  } else {
    return false;
  }
  return true;
}

void TaskMonitor::dump() const {
  // clang-format off
  std::cout << ">> TaskMonitor dump: \n"
            << "name: "<< name << "\n"
            << "id: "<< id << "\n"
            << "usage: " << usage << "%" << std::endl;
  // clang-format on
}

}  // namespace cpu_monitor
