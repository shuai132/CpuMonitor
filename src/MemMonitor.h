#pragma once

#include <unistd.h>

#include <cstdint>

#include "Types.h"

namespace cpu_monitor {

class MemMonitor {
 public:
  struct Usage {
    size_t VmPeak;
    size_t VmSize;
    size_t VmHWM;
    size_t VmRSS;
  };

  struct UsageRet {
    bool ok;
    Usage usage;
  };

 public:
  static UsageRet getUsage(PID_t pid = 0);

  static void dumpUsage(PID_t pid = 0);
};

}  // namespace cpu_monitor