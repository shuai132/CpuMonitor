#include <cmath>

#include "Common.h"
#include "Types.h"
#include "log.h"

using namespace cpu_monitor;

using ProcessKey = PID_t;

using ThreadInfosType = std::vector<msg::ThreadInfo>;

struct ThreadInfoKey {
  ThreadInfoKey() = default;
  ThreadInfoKey(TaskId_t id, double usage_sum) : id(id), usage_sum(usage_sum) {}
  TaskId_t id = 0;
  double usage_sum = 0;
  friend inline bool operator<(const ThreadInfoKey& a, const ThreadInfoKey& b) {
    return a.usage_sum > b.usage_sum;
  }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ThreadInfoKey, id, usage_sum);

struct ThreadInfoItem {
  ThreadInfoKey key;
  ThreadInfosType cpuInfos;
  friend inline bool operator<(const ThreadInfoItem& a, const ThreadInfoItem& b) {
    return a.key < b.key;
  }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ThreadInfoItem, key, cpuInfos);

struct ProcessValue {
  std::string name;
  std::vector<ThreadInfoItem> threadInfos;
  std::vector<msg::MemInfo> memInfos;
  uint64_t maxRss = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ProcessValue, threadInfos, memInfos, maxRss);

struct MsgData {
  std::vector<msg::CpuMsg> msg_cpus;
  std::map<ProcessKey, ProcessValue> msg_pids;
  std::map<PID_t, uint32_t> pid_current_thread_num;
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(MsgData, msg_cpus, msg_pids, pid_current_thread_num);

  void clear() {
    msg_cpus.clear();
    msg_pids.clear();
    pid_current_thread_num.clear();
  }

  void createTestData() {
    clear();
    for (int i = 0; i < 1000; ++i) {
      // cpu
      {
        msg::CpuMsg msg;
        msg.timestamps = i;
        msg.ave.name = "cpu";
        msg.ave.usage = (sin((float)i / 10) + 1) * 50;

        for (int j = 0; j < 4; ++j) {
          msg::CpuInfo c;
          c.name = "cpu" + std::to_string(j);
          c.usage = (sin((float)(i + j * 10) / 10) + 1) * 50;
          c.timestamps = i;
          msg.cores.push_back(std::move(c));
        }
        msg_cpus.push_back(std::move(msg));
      }
    }
  }

  void process(msg::CpuMsg msg) {
    msg_cpus.push_back(std::move(msg));
  }

  void process(msg::ProcessMsg msg) {
    for (auto& pInfo : msg.infos) {
      auto& processValue = msg_pids[pInfo.id];
      auto& threadInfos = processValue.threadInfos;
      pid_current_thread_num[pInfo.id] = pInfo.thread_infos.size();
      // thread info
      for (auto& item : pInfo.thread_infos) {
        auto iter = std::find_if(threadInfos.begin(), threadInfos.end(), [&](auto& v) {
          return v.key.id == item.id;
        });
        if (iter != threadInfos.cend()) {
          iter->key.usage_sum += item.usage;
          iter->cpuInfos.push_back(std::move(item));
        } else {
          ThreadInfoKey key{(TaskId_t)item.id, item.usage};
          ThreadInfosType value;
          value.push_back(std::move(item));
          threadInfos.push_back(ThreadInfoItem{key, std::move(value)});
        }
      }
      std::sort(threadInfos.begin(), threadInfos.end());

      // mem info
      processValue.memInfos.push_back(pInfo.mem_info);
      processValue.maxRss = std::max(processValue.maxRss, pInfo.mem_info.rss);
    }
  }
};
