#pragma once

#include <cstdint>

#include "src/serialize.hpp"

namespace cpu_monitor {

static const uint32_t MessageMaxByteSize = 1024 * 1024 * 10;

}  // namespace cpu_monitor

namespace cpu_monitor {
namespace msg {
struct CpuInfo {
  std::string name{};
  float usage = 0.0f;
  uint64_t timestamps = 0;
};
}  // namespace msg
}  // namespace cpu_monitor
RPC_CORE_DEFINE_TYPE(cpu_monitor::msg::CpuInfo, name, usage, timestamps);

namespace cpu_monitor {
namespace msg {
struct CpuMsg {
  CpuInfo ave{};
  std::vector<CpuInfo> cores{};
  uint64_t timestamps = 0;
};
}  // namespace msg
}  // namespace cpu_monitor
RPC_CORE_DEFINE_TYPE(cpu_monitor::msg::CpuMsg, ave, cores, timestamps);

namespace cpu_monitor {
namespace msg {
struct ThreadInfo {
  std::string name{};
  uint64_t id = 0;
  float usage = 0.0f;
  uint64_t timestamps = 0;
};
}  // namespace msg
}  // namespace cpu_monitor
RPC_CORE_DEFINE_TYPE(cpu_monitor::msg::ThreadInfo, name, id, usage, timestamps);

namespace cpu_monitor {
namespace msg {

struct MemInfo {
  uint64_t peak = 0;
  uint64_t size = 0;
  uint64_t hwm = 0;
  uint64_t rss = 0;
  uint64_t timestamps = 0;
};

}  // namespace msg
}  // namespace cpu_monitor

RPC_CORE_DEFINE_TYPE(cpu_monitor::msg::MemInfo, peak, size, hwm, rss, timestamps);
namespace cpu_monitor {
namespace msg {

struct ProcessInfo {
  uint64_t id = 0;
  std::string name{};
  std::vector<ThreadInfo> thread_infos{};
  MemInfo mem_info{};
};
}  // namespace msg
}  // namespace cpu_monitor
RPC_CORE_DEFINE_TYPE(cpu_monitor::msg::ProcessInfo, id, name, thread_infos, mem_info);

namespace cpu_monitor {
namespace msg {
struct ProcessMsg {
  std::vector<ProcessInfo> infos{};
  uint64_t timestamps = 0;
};
}  // namespace msg
}  // namespace cpu_monitor
RPC_CORE_DEFINE_TYPE(cpu_monitor::msg::ProcessMsg, infos, timestamps);
