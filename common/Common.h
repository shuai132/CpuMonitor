#pragma once

#include <cstdint>

#include "nlohmann/json.hpp"

#define MSG_SERIALIZE_DEFINE(Type, ...) NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Type, __VA_ARGS__)

namespace cpu_monitor {

static const uint32_t MessageMaxByteSize = 1024 * 1024 * 10;

namespace msg {

struct CpuInfo {
  std::string name;
  float usage = 0.0f;
  uint64_t timestamps = 0;
};
MSG_SERIALIZE_DEFINE(CpuInfo, name, usage, timestamps);

struct CpuMsg {
  CpuInfo ave{};
  std::vector<CpuInfo> cores{};
};
MSG_SERIALIZE_DEFINE(CpuMsg, ave, cores);

struct ThreadInfo {
  std::string name;
  uint64_t id = 0;
  float usage = 0.0f;
  uint64_t timestamps = 0;
};
MSG_SERIALIZE_DEFINE(ThreadInfo, name, id, usage, timestamps);

struct MemInfo {
  uint64_t peak = 0;
  uint64_t size = 0;
  uint64_t hwm = 0;
  uint64_t rss = 0;
  uint64_t timestamps = 0;
};
MSG_SERIALIZE_DEFINE(MemInfo, peak, size, hwm, rss, timestamps);

struct ProcessInfo {
  uint64_t id = 0;
  std::string name;
  std::vector<ThreadInfo> thread_infos{};
  MemInfo mem_info{};
};
MSG_SERIALIZE_DEFINE(ProcessInfo, id, name, thread_infos, mem_info);

struct ProcessMsg {
  std::vector<ProcessInfo> infos{};
  uint64_t timestamps = 0;
};
MSG_SERIALIZE_DEFINE(ProcessMsg, infos, timestamps);

}  // namespace msg
}  // namespace cpu_monitor
