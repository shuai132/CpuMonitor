#include <cmath>

#include "Common.h"
#include "Types.h"
#include "log.h"

using namespace cpu_monitor;

using ThreadInfoKey = TaskId_t;
using ThreadInfosType = std::vector<msg::ThreadInfo>;
struct ThreadInfoItem {
  ThreadInfoKey id = 0;
  double usage_sum = 0;
  ThreadInfosType cpu_infos;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ThreadInfoItem, id, usage_sum, cpu_infos);

using ProcessKey = PID_t;
struct ProcessValue {
  std::string name;
  std::vector<ThreadInfoItem> thread_infos;
  std::vector<msg::MemInfo> mem_infos;
  uint64_t max_rss = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ProcessValue, thread_infos, mem_infos, max_rss);

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
        msg.ave.timestamps = i;
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
      auto& threadInfos = processValue.thread_infos;
      pid_current_thread_num[pInfo.id] = pInfo.thread_infos.size();
      // thread info
      for (auto& item : pInfo.thread_infos) {
        auto iter = std::find_if(threadInfos.begin(), threadInfos.end(), [&](auto& v) {
          return v.id == item.id;
        });
        if (iter != threadInfos.cend()) {
          iter->usage_sum += item.usage;
          iter->cpu_infos.push_back(std::move(item));
        } else {
          ThreadInfoKey key = item.id;
          ThreadInfosType value;
          value.push_back(item);
          threadInfos.push_back(ThreadInfoItem{key, item.usage, std::move(value)});
        }
      }
      std::sort(threadInfos.begin(), threadInfos.end(), [](auto& a, auto& b) {
        return a.usage_sum > b.usage_sum;
      });

      // mem info
      processValue.name = pInfo.name;
      processValue.mem_infos.push_back(pInfo.mem_info);
      processValue.max_rss = std::max(processValue.max_rss, pInfo.mem_info.rss);
    }
  }
};
