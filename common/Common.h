#pragma once

#include <cstdint>

#include "rpc_core/serialize.hpp"

// #define MSG_SERIALIZE_USE_JSON
#ifdef MSG_SERIALIZE_USE_JSON
#include "rpc_core/plugin/json_msg.hpp"
#define MSG_SERIALIZE_DEFINE(...) RPC_CORE_DEFINE_TYPE_JSON(__VA_ARGS__)
#else
#ifdef MSG_SERIALIZE_SUPPORT_TO_JSON
#include "nlohmann/json.hpp"
#define MSG_SERIALIZE_DEFINE(Type, ...)   \
  RPC_CORE_DEFINE_TYPE(Type, __VA_ARGS__) \
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Type, __VA_ARGS__)
#else
#define MSG_SERIALIZE_DEFINE(Type, ...) RPC_CORE_DEFINE_TYPE(Type, __VA_ARGS__)
#endif
#endif

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
  uint64_t timestamps = 0;
};
MSG_SERIALIZE_DEFINE(CpuMsg, ave, cores, timestamps);

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
