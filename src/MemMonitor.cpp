#include "MemMonitor.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "detail/defer.h"

namespace cpu_monitor {

MemMonitor::UsageRet MemMonitor::getUsage(PID_t pid) {
  if (pid == 0) pid = getpid();
  char filePath[64];
  snprintf(filePath, sizeof(filePath), "/proc/%d/status", pid);

  UsageRet usageRet;  // NOLINT
  auto &result = usageRet.usage;

  char buf[64];
  auto getValue = [&buf] {
    char *startPos = nullptr;
    char *endPos = nullptr;

    char *p = buf;
    do {
      char c = *p;
      if (c == 0) break;
      if (startPos != nullptr) {
        if (c == ' ') endPos = p;
      } else if (c >= '0' && c <= '9') {
        startPos = p;
      }
    } while (p++);
    *endPos = 0;
    return std::strtol(startPos, nullptr, 10);
  };
  auto startWith = [&buf](const char *str) {
    return strncmp(buf, str, strlen(str)) == 0;
  };

  FILE *fp = fopen(filePath, "r");
  defer {
    if (fp == nullptr) return;
    fclose(fp);
  };

  if (fp == nullptr) {
    usageRet.ok = false;
  } else {
    usageRet.ok = true;
    while (!feof(fp)) {
      auto r = fgets(buf, sizeof(buf), fp);
      (void)r;
      if (startWith("VmPeak:")) {
        result.VmPeak = getValue();
      } else if (startWith("VmSize:")) {
        result.VmSize = getValue();
      } else if (startWith("VmHWM:")) {
        result.VmHWM = getValue();
      } else if (startWith("VmRSS:")) {
        result.VmRSS = getValue();
      }
    }
  }
  return usageRet;
}

void MemMonitor::dumpUsage(PID_t pid) {
  if (pid == 0) pid = getpid();
  char filePath[64];
  snprintf(filePath, sizeof(filePath), "/proc/%d/status", pid);
  printf("\nMemMonitor:\n");
  char buf[1024];
  FILE *fp = fopen(filePath, "r");
  while (!feof(fp)) {
    auto r = fgets(buf, sizeof(buf), fp);
    (void)r;
    printf("%s", buf);
  }
  fclose(fp);
}

}  // namespace cpu_monitor
