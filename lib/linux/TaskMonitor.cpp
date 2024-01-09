#include "TaskMonitor.h"

#include <fstream>
#include <iostream>

#include "TaskStat.h"

#define READ_PROP(name) file >> stat.name

namespace cpu_monitor {

TaskMonitor::TaskMonitor(TaskId_t tid, TaskMonitor::TotalTimeImpl cpuTicksImpl) : id(tid), totalTimeImpl_(std::move(cpuTicksImpl)) {
  update();
}

bool TaskMonitor::update() {
  std::string path = "/proc/" + std::to_string(id) + "/task/" + std::to_string(id) + "/stat";
  std::fstream file(path, std::fstream::in);
  if (!file.is_open()) {
    return false;
  }

  detail::TaskStat stat;
  READ_PROP(id);
  READ_PROP(name);
  READ_PROP(task_state);
  READ_PROP(ppid);
  READ_PROP(pgid);
  READ_PROP(sid);
  READ_PROP(tty_nr);
  READ_PROP(tty_pgrp);
  READ_PROP(task_flags);
  READ_PROP(min_flt);
  READ_PROP(cmin_flt);
  READ_PROP(maj_flt);
  READ_PROP(cmaj_flt);
  READ_PROP(utime);
  READ_PROP(stime);
  READ_PROP(cutime);
  READ_PROP(cstime);
  READ_PROP(priority);
  READ_PROP(nice);
  READ_PROP(num_threads);
  READ_PROP(it_real_value);
  READ_PROP(start_time);
  READ_PROP(vsize);
  READ_PROP(rss);
  READ_PROP(rlim);
  READ_PROP(start_code);
  READ_PROP(end_code);
  READ_PROP(start_stack);
  READ_PROP(kstkesp);
  READ_PROP(kstkeip);
  READ_PROP(pendingsig);
  READ_PROP(block_sig);
  READ_PROP(sigign);
  READ_PROP(sigcatch);
  READ_PROP(wchan);
  READ_PROP(nswap);
  READ_PROP(cnswap);
  READ_PROP(exit_signal);
  READ_PROP(task_cpu);
  READ_PROP(task_rt_priority);
  READ_PROP(task_policy);

  // linux thread name like: (cpu_monitor)
  // remove the `()`
  if (stat.name.size() >= 2 && stat.name[0] == '(') {
    name = stat.name.substr(1, stat.name.size() - 2);
  } else {
    name = stat.name;
  }

  // skip calculate usage
  if (!totalTimeImpl_) return false;

  auto totalThreadTicksNow = stat.calcTicksTotal();
  auto deltaThread = totalThreadTicksNow - totalThreadTime_;
  usage = deltaThread * 100.f / totalTimeImpl_();  // NOLINT
  if (usage > 100) {                               // invalid data
    usage = 0;
  }
  totalThreadTime_ = totalThreadTicksNow;
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
