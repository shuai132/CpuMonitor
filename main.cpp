#include <unistd.h>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace cpu_monitor {

namespace TICK {
enum : int {
  USER = 0,
  NICE,
  SYSTEM,
  IDLE,
  IOWAIT,
  IRQ,
  SOFTIRQ,
  STEAL,
  GUEST,
  GUEST_NICE,
  TYPE_NUM,
};
}

struct CpuTick {
  char name[16];
  uint64_t ticks[TICK::TYPE_NUM];

  uint64_t calcTicksIdle() {
    return ticks[TICK::IDLE] + ticks[TICK::IOWAIT];
  }

  uint64_t calcTicksTotal() {
    uint64_t total = 0;
    for (auto tick : ticks) total += tick;
    return total;
  }
};

struct CpuTickThread {
  uint64_t id;        // 进程(包括轻量级进程，即线程)号
  std::string name;   // 应用程序或命令的名字。
  char task_state;    // 任务的状态
                      // R:runnign
                      // S:sleeping (TASK_INTERRUPTIBLE)
                      // D:disk sleep (TASK_UNINTERRUPTIBLE)
                      // T:stopped
                      // T:tracing stop
                      // Z:zombie
                      // X:dead
  uint64_t ppid,      // 父进程ID
      pgid,           // 线程组号
      sid,            // 该任务所在的会话组ID
      tty_nr,         // 该任务的 tty 终端的设备号，INT(/)= 主设备号，(-主设备号)= 次设备号。
      tty_pgrp,       // 终端的进程组号，当前运行在该任务所在终端的前台任务(包括 shell 应用程序)的 PID。
      task_flags,     // 进程标志位，查看该任务的特性。
      min_flt,        // 该任务不需要从硬盘拷数据而发生的缺页(次缺页)的次数。
      cmin_flt,       // 累计的该任务的所有的 waited-for 进程曾经发生的次缺页的次数目。
      maj_flt,        // 该任务需要从硬盘拷数据而发生的缺页(主缺页)的次数。
      cmaj_flt,       // 累计的该任务的所有的 waited-for 进程曾经发生的主缺页的次数目。
      utime,          // 该任务在用户态运行的时间，单位为 jiffies。
      stime,          // 该任务在核心态运行的时间，单位为 jiffies。
      cutime,         // 累计的该任务的所有的 waited-for 进程曾经在用户态运行的时间，单位为 jiffies。
      cstime,         // 累计的该任务的所有的 waited-for 进程曾经在核心态运行的时间，单位为 jiffies。
      priority,       // 任务的动态优先级。
      nice,           // 任务的静态优先级。
      num_threads,    // 该任务所在的线程组里线程的个数。
      it_real_value,  // 由于计时间隔导致的下一个 SIGALRM 发送进程的时延，以 jiffy 为单位。
      start_time,     // 该任务启动的时间，单位为 jiffies。
      vsize,          // (page)该任务的虚拟地址空间大小。
      rss,  //(page) 该任务当前驻留物理地址空间的大小；Number of pages the process has in real memory,minu  for administrative purpose.
            // 这些页可能用于代码，数据和栈。
      rlim,              // (bytes)该任务能驻留物理地址空间的最大值。
      start_code,        // 该任务在虚拟地址空间的代码段的起始地址。
      end_code,          // 该任务在虚拟地址空间的代码段的结束地址。
      start_stack,       // 该任务在虚拟地址空间的栈的结束地址。
      kstkesp,           // esp(位堆栈指针)的当前值,与在进程的内核堆栈页得到的一致。
      kstkeip,           // 指向将要执行的指令的指针,EIP(位指令指针)的当前值。
      pendingsig,        // 待处理信号的位图，记录发送给进程的普通信号。
      block_sig,         // 阻塞信号的位图。
      sigign,            // 忽略的信号的位图。
      sigcatch,          // 被俘获的信号的位图。
      wchan,             // 如果该进程是睡眠状态，该值给出调度的调用点。
      nswap,             // 被swapped的页数，当前没用。
      cnswap,            // 所有子进程被swapped的页数的和，当前没用。
      exit_signal,       // 该进程结束时，向父进程所发送的信号。
      task_cpu,          // 运行在哪个CPU上。
      task_rt_priority,  // 实时进程的相对优先级别。
      task_policy;       // 进程的调度策略，=非实时进程，=FIFO实时进程；=RR实时进程

  uint64_t calcTicksTotal() {
    return utime + stime + cutime + cstime;
  }
};

class CpuMonitor {
 public:
  explicit CpuMonitor() {
    update();
  }

  void update() {
    invertAB();
    updateFromStat();
    calcUsage();
  }

 private:
  void invertAB() {
    currentA = !currentA;
  }

  CpuTick &cpuTickCurr() {
    return currentA ? cpuTickA : cpuTickB;
  }

  CpuTick &cpuTickPre() {
    return !currentA ? cpuTickA : cpuTickB;
  }

  void updateFromStat() {
    FILE *stat_fp = fopen("/proc/stat", "r");
    auto &cpuTick = cpuTickCurr();
    // clang-format off
    fscanf(stat_fp, // NOLINT
           "%s"
           " %" PRIu64
           " %" PRIu64
           " %" PRIu64
           " %" PRIu64
           " %" PRIu64
           " %" PRIu64
           " %" PRIu64
           " %" PRIu64
           " %" PRIu64
           " %" PRIu64
           "\n",
           cpuTick.name,
           &(cpuTick.ticks[TICK::USER]),
           &(cpuTick.ticks[TICK::NICE]),
           &(cpuTick.ticks[TICK::SYSTEM]),
           &(cpuTick.ticks[TICK::IDLE]),
           &(cpuTick.ticks[TICK::IOWAIT]),
           &(cpuTick.ticks[TICK::IRQ]),
           &(cpuTick.ticks[TICK::SOFTIRQ]),
           &(cpuTick.ticks[TICK::STEAL]),
           &(cpuTick.ticks[TICK::GUEST]),
           &(cpuTick.ticks[TICK::GUEST_NICE]));
    // clang-format on
    fclose(stat_fp);
  }

  void calcUsage() {
    totalTime = cpuTickCurr().calcTicksTotal() - cpuTickPre().calcTicksTotal();
    idleTime = cpuTickCurr().calcTicksIdle() - cpuTickPre().calcTicksIdle();
    uint64_t active = totalTime - idleTime;
    usage = active * 100.f / totalTime;  // NOLINT
  }

 public:
  float usage{};
  uint64_t idleTime{};
  uint64_t totalTime{};

 private:
  bool currentA{};
  CpuTick cpuTickA{};
  CpuTick cpuTickB{};
};

class CpuMonitorThread {
 public:
  explicit CpuMonitorThread(std::string path, CpuMonitor *monitor) {
    statPath = std::move(path);
    cpuMonitor = monitor;
    update();
  }

  void update() {
    invertAB();
    updateFromStat();
    calcUsage();
  }

  void dump() {
    auto &stat = cpuTickCurr();
    // clang-format off
    std::cout << "dump: \n"
              << "id: " << stat.id << "\n"
              << "name: " << stat.name << "\n"
              << "task_state: " << stat.task_state << "\n"
              << "utime: " << stat.utime << "\n"
              << "stime: " << stat.stime << "\n"
              << "cutime: " << stat.cutime << "\n"
              << "cstime: " << stat.cstime << "\n"
              << "num_threads: " << stat.num_threads << "\n"
              << "rss: " << stat.rss << "\n"
              << "task_cpu: " << stat.task_cpu << "\n"
              ;
    // clang-format on
  }

 private:
  void invertAB() {
    currentA = !currentA;
  }

  CpuTickThread &cpuTickCurr() {
    return currentA ? cpuTickA : cpuTickB;
  }

  CpuTickThread &cpuTickPre() {
    return !currentA ? cpuTickA : cpuTickB;
  }

  void updateFromStat() {
    std::fstream file(statPath.c_str());
    auto &stat = cpuTickCurr();
    file >> stat.id;
    file >> stat.name;
    file >> stat.task_state;
    file >> stat.ppid;
    file >> stat.pgid;
    file >> stat.sid;
    file >> stat.tty_nr;
    file >> stat.tty_pgrp;
    file >> stat.task_flags;
    file >> stat.min_flt;
    file >> stat.cmin_flt;
    file >> stat.maj_flt;
    file >> stat.cmaj_flt;
    file >> stat.utime;
    file >> stat.stime;
    file >> stat.cutime;
    file >> stat.cstime;
    file >> stat.priority;
    file >> stat.nice;
    file >> stat.num_threads;
    file >> stat.it_real_value;
    file >> stat.start_time;
    file >> stat.vsize;
    file >> stat.rss;
    file >> stat.rlim;
    file >> stat.start_code;
    file >> stat.end_code;
    file >> stat.start_stack;
    file >> stat.kstkesp;
    file >> stat.kstkeip;
    file >> stat.pendingsig;
    file >> stat.block_sig;
    file >> stat.sigign;
    file >> stat.sigcatch;
    file >> stat.wchan;
    file >> stat.nswap;
    file >> stat.cnswap;
    file >> stat.exit_signal;
    file >> stat.task_cpu;
    file >> stat.task_rt_priority;
    file >> stat.task_policy;
  }

  void calcUsage() {
    totalThreadTime = cpuTickCurr().calcTicksTotal() - cpuTickPre().calcTicksTotal();
    totalCpuTime = cpuMonitor->totalTime;
    usage = totalThreadTime * 100.f / totalCpuTime;  // NOLINT
  }

 public:
  float usage{};
  uint64_t totalThreadTime{};
  uint64_t totalCpuTime{};

 private:
  CpuMonitor *cpuMonitor;

  bool currentA{};
  CpuTickThread cpuTickA{};
  CpuTickThread cpuTickB{};

  std::string statPath;
};
}  // namespace cpu_monitor

using namespace cpu_monitor;

int main(int ac, char **av) {
  CpuMonitor cpuMonitor;
  CpuMonitorThread threadMonitor("/proc/859/task/8081/stat", &cpuMonitor);
  for (;;) {
    sleep(1);
    cpuMonitor.update();
    threadMonitor.update();
    threadMonitor.dump();
    printf("usage: %.2f\n", threadMonitor.usage);
  }
}
