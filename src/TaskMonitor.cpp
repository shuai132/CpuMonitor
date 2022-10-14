#include "TaskMonitor.h"

#include <fstream>
#include <iostream>

#define READ_PROP(name) file >> stat.name
#define DUMP_PROP(name) #name ": " << stat.name << "\n"

namespace cpu_monitor {

TaskMonitor::TaskMonitor(std::string path, TotalTimeImpl totalTimeImpl) {
  statPath_ = std::move(path);
  totalTimeImpl_ = std::move(totalTimeImpl);
  update();
}

bool TaskMonitor::update() {
  invertAB();
  bool ret = updateFromStat();
  if (!ret) {
    return false;
  }
  calcUsage();
  return true;
}

void TaskMonitor::dump() {
  auto &stat = taskStatCurr();
  // clang-format off
  std::cout << ">> TaskMonitor dump: \n"
            << DUMP_PROP(id)
            << DUMP_PROP(name)
            << DUMP_PROP(task_state)
            << DUMP_PROP(utime)
            << DUMP_PROP(stime)
            << DUMP_PROP(cutime)
            << DUMP_PROP(cstime)
            << DUMP_PROP(num_threads)
            << DUMP_PROP(rss)
            << DUMP_PROP(task_cpu)
            << DUMP_PROP(task_policy)
            << "usage: " << usage << "%" << std::endl;
  // clang-format on
}

TaskStat &TaskMonitor::stat() {
  return taskStatCurr();
}

void TaskMonitor::invertAB() {
  currentA_ = !currentA_;
}

TaskStat &TaskMonitor::taskStatCurr() {
  return currentA_ ? cpuTickA_ : cpuTickB_;
}

TaskStat &TaskMonitor::taskStatPre() {
  return !currentA_ ? cpuTickA_ : cpuTickB_;
}

bool TaskMonitor::updateFromStat() {
  std::fstream file(statPath_.c_str(), std::fstream::in);
  if (!file.is_open()) {
    return false;
  }

  auto &stat = taskStatCurr();
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

  return true;
}

void TaskMonitor::calcUsage() {
  totalThreadTime = taskStatCurr().calcTicksTotal() - taskStatPre().calcTicksTotal();
  totalCpuTime = totalTimeImpl_();
  usage = totalThreadTime * 100.f / totalCpuTime;  // NOLINT
}

}  // namespace cpu_monitor
