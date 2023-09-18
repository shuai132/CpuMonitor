#pragma once

#include <cstdint>

#include "rpc_core/serialize.hpp"

namespace cpu_monitor {

static const uint32_t MessageMaxByteSize = 1024 * 1024 * 10;

namespace msg {

struct CpuInfo {
  std::string name{};
  float usage = 0.0f;
  uint64_t timestamps = 0;
};
RPC_CORE_DEFINE_TYPE(CpuInfo, name, usage, timestamps);

struct CpuMsg {
  CpuInfo ave{};
  std::vector<CpuInfo> cores{};
  uint64_t timestamps = 0;
};
RPC_CORE_DEFINE_TYPE(CpuMsg, ave, cores, timestamps);

struct ThreadInfo {
  std::string name{};
  uint64_t id = 0;
  float usage = 0.0f;
  uint64_t timestamps = 0;
};
RPC_CORE_DEFINE_TYPE(ThreadInfo, name, id, usage, timestamps);

struct MemInfo {
  uint64_t peak = 0;
  uint64_t size = 0;
  uint64_t hwm = 0;
  uint64_t rss = 0;
  uint64_t timestamps = 0;
};
RPC_CORE_DEFINE_TYPE(MemInfo, peak, size, hwm, rss, timestamps);

struct ProcessInfo {
  uint64_t id = 0;
  std::string name{};
  std::vector<ThreadInfo> thread_infos{};
  MemInfo mem_info{};
};
RPC_CORE_DEFINE_TYPE(ProcessInfo, id, name, thread_infos, mem_info);

struct ProcessMsg {
  std::vector<ProcessInfo> infos{};
  uint64_t timestamps = 0;
};
RPC_CORE_DEFINE_TYPE(ProcessMsg, infos, timestamps);

}  // namespace msg
}  // namespace cpu_monitor
