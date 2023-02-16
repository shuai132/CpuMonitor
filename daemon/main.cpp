#include <unistd.h>

#include <algorithm>
#include <memory>

#include "Common.h"
#include "CpuMonitor.h"
#include "CpuMsg_generated.h"
#include "MemMonitor.h"
#include "ProgressMsg_generated.h"
#include "RpcMsg.h"
#include "TaskMonitor.h"
#include "Utils.h"
#include "asio.hpp"
#include "log.h"
#include "rpc_server.hpp"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

using namespace cpu_monitor;

// all argv
struct {
  bool h = false;
  uint32_t d_update_interval_ms = 1000;
  bool s_run_server = false;
  uint32_t s_server_port = 8088;
  bool c_only_monitor_cpu = false;
} s_argv;

// main logic
static std::unique_ptr<asio::io_context> s_context;
static std::unique_ptr<asio::steady_timer> s_timer_update;

// rpc
static std::unique_ptr<asio_net::rpc_server> s_rpc_server;
static std::shared_ptr<RpcCore::Rpc> s_rpc;

// cpu monitor
static std::unique_ptr<CpuMonitor> s_monitor_cpu = std::make_unique<CpuMonitor>();
static const auto s_total_time_impl = [] {
  return s_monitor_cpu->ave->totalTime / s_monitor_cpu->cores.size();
};

using MonitorTasks = std::vector<std::unique_ptr<TaskMonitor>>;

struct ProgressValue {
  MonitorTasks tasks;
  MemMonitor::Usage memUsage;
};
struct ProgressKey {
  PID_t pid;
  std::string name;

  friend inline bool operator<(const ProgressKey& a, const ProgressKey& b) {
    return a.pid < b.pid;
  }
};
using MonitorPids = std::map<ProgressKey, ProgressValue>;
static MonitorPids s_monitor_pids;

static bool addMonitorPid(PID_t pid);
static bool addMonitorPid(const std::string& pid);
static bool addMonitorPidByName(const std::string& name);

static void initRpcTask() {
  using namespace RpcCore;

  s_rpc->subscribe("add_pid", [](const String& pid) -> String {
    LOGD("add_pid: %s", pid.c_str());

    auto iter = std::find_if(s_monitor_pids.begin(), s_monitor_pids.end(), [&](auto& p) {
      return std::to_string(p.first.pid) == pid;
    });
    if (iter != s_monitor_pids.cend()) {
      return "already added";
    }

    if (addMonitorPid(pid)) {
      return "ok";
    } else {
      return "no such pid";
    }
  });

  s_rpc->subscribe("del_pid", [](const String& pid) -> String {
    LOGD("del_pid: %s", pid.c_str());
    auto iter = std::find_if(s_monitor_pids.begin(), s_monitor_pids.end(), [&](const auto& item) {
      return std::to_string(item.first.pid) == pid;
    });
    if (iter != s_monitor_pids.cend()) {
      s_monitor_pids.erase(iter);
      return "ok";
    } else {
      return "no such pid";
    }
  });

  s_rpc->subscribe("add_name", [](const String& name) -> String {
    LOGD("add_name: %s", name.c_str());
    auto iter = std::find_if(s_monitor_pids.begin(), s_monitor_pids.end(), [&](auto& p) {
      return p.first.name == name;
    });
    if (iter != s_monitor_pids.cend()) {
      return "already added";
    }

    if (addMonitorPidByName(name)) {
      return "ok";
    } else {
      return "no such name";
    }
  });

  s_rpc->subscribe("del_name", [](const String& name) -> String {
    LOGD("del_name: %s", name.c_str());
    auto iter = std::find_if(s_monitor_pids.begin(), s_monitor_pids.end(), [&](const auto& item) {
      return item.first.name == name;
    });
    if (iter != s_monitor_pids.cend()) {
      s_monitor_pids.erase(iter);
      return "ok";
    } else {
      return "no such pid";
    }
  });

  s_rpc->subscribe("get_added_pids", [] {
    RpcMsg<msg::ProgressMsgT> msg;
    for (const auto& monitorPid : s_monitor_pids) {
      auto& id = monitorPid.first;
      auto progressInfo = std::make_unique<msg::ProgressInfoT>();
      progressInfo->id = id.pid;
      progressInfo->name = id.name;
      msg->infos.push_back(std::move(progressInfo));
    }
    return msg;
  });

  s_rpc->subscribe("monitor_all", [] {

  });

  s_rpc->subscribe("set_update_interval", [] {

  });
}

static void sendNowCpuInfos() {
  if (!s_rpc) return;
  auto timestampsNow = utils::getTimestamps();

  // cpu info
  {
    RpcMsg<msg::CpuMsgT> msg;
    // ave
    {
      auto info = std::make_unique<msg::CpuInfoT>();
      info->name = s_monitor_cpu->ave->name;
      info->usage = s_monitor_cpu->ave->usage;
      info->timestamps = timestampsNow;
      msg->ave = std::move(info);
    }
    // cores
    for (const auto& core : s_monitor_cpu->cores) {
      auto info = std::make_unique<msg::CpuInfoT>();
      info->name = core->name;
      info->usage = core->usage;
      info->timestamps = timestampsNow;
      msg->cores.push_back(std::move(info));
    }
    msg->timestamps = timestampsNow;
    s_rpc->cmd("on_cpu_msg")->msg(msg)->call();
  }

  // progress info
  {
    RpcMsg<msg::ProgressMsgT> msg;
    for (const auto& monitorPid : s_monitor_pids) {
      auto& id = monitorPid.first;
      auto& tasks = monitorPid.second.tasks;
      auto& memUsage = monitorPid.second.memUsage;

      auto progressInfo = std::make_unique<msg::ProgressInfoT>();
      progressInfo->id = id.pid;
      progressInfo->name = id.name;

      // mem info
      {
        auto mem = std::make_unique<msg::MemInfoT>();
        mem->peak = memUsage.VmPeak;
        mem->size = memUsage.VmSize;
        mem->hwm = memUsage.VmHWM;
        mem->rss = memUsage.VmRSS;
        mem->timestamps = timestampsNow;
        progressInfo->mem_info = std::move(mem);
      }

      for (const auto& task : tasks) {
        auto taskInfo = std::make_unique<msg::ThreadInfoT>();
        taskInfo->id = task->id;
        taskInfo->name = task->name;
        taskInfo->usage = task->usage;
        taskInfo->timestamps = timestampsNow;
        progressInfo->thread_infos.push_back(std::move(taskInfo));
      }
      msg->infos.push_back(std::move(progressInfo));
    }
    msg->timestamps = timestampsNow;
    s_rpc->cmd("on_progress_msg")->msg(msg)->call();
  }
}

static void runServer() {
  using namespace asio_net;
  s_rpc_server = std::make_unique<rpc_server>(*s_context, s_argv.s_server_port, MessageMaxByteSize);
  auto& server = s_rpc_server;
  server->on_session = [](const std::weak_ptr<rpc_session>& ws) {
    LOGD("on_session");
    if (s_rpc) {
      ws.lock()->close();
      return;
    }

    auto session = ws.lock();
    session->on_close = [] {
      LOGD("session: on_close");
      s_rpc = nullptr;
    };

    s_rpc = session->rpc;
    initRpcTask();
  };

  LOGI("start server: port: %d", s_argv.s_server_port);
  server->start(true);
}

static void updateCpu() {
  s_monitor_cpu->update(true);
  printf("system %s usage: %.2f%%\n", s_monitor_cpu->ave->name.c_str(), s_monitor_cpu->ave->usage);
}

static void updateProgress() {
  for (auto& item : s_monitor_pids) {
    auto& tasks = item.second.tasks;
    auto& memUsage = item.second.memUsage;
    auto memUsageRet = MemMonitor::getUsage(item.first.pid);
    if (!memUsageRet.ok) {
      printf("progress exit: name: %-15s, id: %-7" PRIu32 "\n", item.first.name.c_str(), item.first.pid);
      continue;
    }

    memUsage = memUsageRet.usage;
    printf("VmPeak: %8zu kB\n", memUsage.VmPeak);
    printf("VmSize: %8zu kB\n", memUsage.VmSize);
    printf("VmHWM:  %8zu kB\n", memUsage.VmHWM);
    printf("VmRSS:  %8zu kB\n", memUsage.VmRSS);
    for (auto& task : tasks) {
      bool ok = task->update();
      if (ok) {
        printf("name: %-15s, id: %-7" PRIu32 ", usage: %.2f%%\n", task->name.c_str(), task->id, task->usage);
      } else {
        printf("thread exit: name: %s, id: %" PRIu32 "\n", task->name.c_str(), task->id);
      }
    }
    printf("\n");
  }
}

static bool updateProgressChange() {
  for (auto& monitorPid : s_monitor_pids) {
    auto& id = monitorPid.first;
    auto pid = id.pid;
    auto& tasks = monitorPid.second.tasks;

    const auto& taskRet = Utils::getTasksOfPid(pid);
    if (!taskRet.ok) {
      printf("progress exit! will wait...\n");
      return false;
    }
    const auto& tasksNow = taskRet.ids;

    // 删除已不存在的线程
    const auto isTaskAlive = [&](TaskId_t taskId) {
      return std::find(tasksNow.cbegin(), tasksNow.cend(), taskId) != tasksNow.cend();
    };
    for (auto iter = tasks.begin(); iter != tasks.cend();) {
      // delete not cared task
      if (!isTaskAlive(iter->get()->id)) {
        iter = tasks.erase(iter);
      } else {
        ++iter;
      }
    }

    // 添加新增的线程
    const auto isNewTask = [&](TaskId_t taskId) {
      return std::find_if(tasks.cbegin(), tasks.cend(), [taskId](const std::unique_ptr<TaskMonitor>& monitor) {
               return monitor->id == taskId;
             }) == tasks.cend();
    };
    for (const auto& tid : tasksNow) {
      if (isNewTask(tid)) {
        tasks.push_back(std::make_unique<TaskMonitor>(tid, s_total_time_impl));
      }
    }
  }

  return true;
}

static bool addMonitorPid(PID_t pid) {
  auto ret = Utils::getTasksOfPid(pid);
  LOGI("get task info: pid: %u, ok: %d, num: %zu", pid, ret.ok, ret.ids.size());
  if (!ret.ok) {
    LOGE("pid not found: %u", pid);
    return false;
  }

  // add to monitor
  auto& monitorTask = s_monitor_pids[{pid, ret.name}];

  // add threads
  for (auto tid : ret.ids) {
    LOGI("thread id: %d", tid);
    monitorTask.tasks.push_back(std::make_unique<TaskMonitor>(tid, s_total_time_impl));
  }

  return true;
}

static bool addMonitorPid(const std::string& pid) {
  PID_t pid_t = strtoul(pid.c_str(), nullptr, 10);
  if (pid_t == 0) {
    return false;
  }
  return addMonitorPid(pid_t);
}

static bool addMonitorPidByName(const std::string& name) {
  auto pid = Utils::getPidByName(name);
  return addMonitorPid(pid);
}

static void asyncNextUpdate() {
  s_timer_update->expires_after(std::chrono::milliseconds(s_argv.d_update_interval_ms));
  s_timer_update->async_wait([](asio::error_code ec) {
    updateCpu();
    updateProgress();
    sendNowCpuInfos();
    updateProgressChange();
    asyncNextUpdate();
  });
}

[[noreturn]] static void monitorCpu() {
  CpuMonitor cpu;
  for (;;) {
    sleep(1);
    cpu.update();
    printf("%s usage: %.2f%%\n", cpu.ave->name.c_str(), cpu.ave->usage);
    for (auto& item : cpu.cores) {
      printf("%s usage: %.2f%%\n", item->name.c_str(), item->usage);
    }
    printf("\n");
  }
}

static void initApp() {
  s_context = std::make_unique<asio::io_context>();
  s_timer_update = std::make_unique<asio::steady_timer>(*s_context);
  asyncNextUpdate();
}

static void runApp() {
  s_context->run();
}

static void showHelp() {
  printf(R"(Usage:
-h : 打印此帮助
-d : 刷新间隔/ms 默认1000
-s : 以服务方式启动 配合GUI使用
-p : 指定开启的服务端口号
-c : 仅在终端打印所有CPU核使用率
-i : 指定监控的PID 半角逗号分隔
-n : 指定监控进程名 半角逗号分隔
)");
}
int main(int argc, char** argv) {
  if (argc < 2) {
    showHelp();
    return 0;
  }

  s_monitor_cpu = std::make_unique<CpuMonitor>();

  int ret;
  while ((ret = getopt(argc, argv, "h::d:s::p:c::i:n:")) != -1) {
    switch (ret) {
      case 'h': {
        s_argv.h = true;
      } break;
      case 'd': {
        s_argv.d_update_interval_ms = std::stoul(optarg, nullptr, 10);
        LOGD("update_interval_ms: %u", s_argv.d_update_interval_ms);
      } break;
      case 's': {
        s_argv.s_run_server = true;
      } break;
      case 'p': {
        s_argv.s_server_port = std::stoul(optarg, nullptr, 10);
        LOGD("server_port: %u", s_argv.s_server_port);
      } break;
      case 'c': {
        s_argv.c_only_monitor_cpu = true;
      } break;
      case 'i': {
        auto& allPids = optarg;
        for (const auto& pidStr : string_utils::Split(allPids, ",", true)) {
          auto pid = std::stoi(pidStr, nullptr, 10);
          LOGD("add pid: %d", pid);
          addMonitorPid(pid);
        }
      } break;
      case 'n': {
        auto& allNames = optarg;
        for (const auto& name : string_utils::Split(allNames, ",", true)) {
          LOGD("add name: %s", name.c_str());
          addMonitorPidByName(name);
        }
      } break;
      default: {
        s_argv.h = true;
      } break;
    }
  }

  if (s_argv.h) {
    showHelp();
    return 0;
  }

  if (s_argv.c_only_monitor_cpu) {
    monitorCpu();
  }

  initApp();
  if (s_argv.s_run_server) {
    runServer();
  } else {
    runApp();
  }
  return 0;
}
