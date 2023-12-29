#include <unistd.h>

#include <algorithm>
#include <memory>
#include <thread>

#include "Common.h"
#include "CpuMonitor.h"
#include "MemMonitor.h"
#include "TaskMonitor.h"
#include "Utils.h"
#include "asio.hpp"
#include "asio_net/rpc_server.hpp"
#include "log.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

using namespace cpu_monitor;

// all argv
struct {
  bool h = false;
  std::string all_names;
  std::string all_pids;
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
static std::shared_ptr<rpc_core::rpc> s_rpc;

// cpu monitor
static std::unique_ptr<CpuMonitor> s_monitor_cpu;
static const auto s_total_time_impl = [] {
  return s_monitor_cpu->ave->totalTime / s_monitor_cpu->cores.size();
};

using MonitorTasks = std::vector<std::unique_ptr<TaskMonitor>>;

struct ProcessValue {
  MonitorTasks tasks;
  MemMonitor::Usage memUsage{};
};
struct ProcessKey {
  PID_t pid;
  std::string name;

  friend inline bool operator<(const ProcessKey& a, const ProcessKey& b) {
    return a.pid < b.pid;
  }
};
using MonitorPids = std::map<ProcessKey, ProcessValue>;
static MonitorPids s_monitor_pids;

static bool addMonitorPid(PID_t pid);
static bool addMonitorPid(const std::string& pid);
static bool addMonitorPidByName(const std::string& name);

static void initRpcTask() {
  using namespace rpc_core;
  s_rpc = rpc::create();

  s_rpc->subscribe("add_pid", [](const std::string& pid) -> std::string {
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

  s_rpc->subscribe("del_pid", [](const std::string& pid) -> std::string {
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

  s_rpc->subscribe("add_name", [](const std::string& name) -> std::string {
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

  s_rpc->subscribe("del_name", [](const std::string& name) -> std::string {
    LOGD("del_name: %s", name.c_str());
    auto iter = std::find_if(s_monitor_pids.begin(), s_monitor_pids.end(), [&](const auto& item) {
      return item.first.name == name;
    });
    if (iter != s_monitor_pids.cend()) {
      s_monitor_pids.erase(iter);
      return "ok";
    } else {
      return "no such name";
    }
  });

  s_rpc->subscribe("get_added_pids", [] {
    msg::ProcessMsg msg;
    for (const auto& monitorPid : s_monitor_pids) {
      auto& id = monitorPid.first;
      msg::ProcessInfo processInfo;
      processInfo.id = id.pid;
      processInfo.name = id.name;
      msg.infos.push_back(std::move(processInfo));
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
    msg::CpuMsg msg;
    // ave
    {
      msg::CpuInfo info;
      info.name = s_monitor_cpu->ave->name;
      info.usage = s_monitor_cpu->ave->usage;
      info.timestamps = timestampsNow;
      msg.ave = std::move(info);
    }
    // cores
    for (const auto& core : s_monitor_cpu->cores) {
      msg::CpuInfo info;
      info.name = core->name;
      info.usage = core->usage;
      info.timestamps = timestampsNow;
      msg.cores.push_back(std::move(info));
    }
    s_rpc->cmd("on_cpu_msg")->msg(msg)->call();
  }

  // process info
  {
    msg::ProcessMsg msg;
    for (const auto& monitorPid : s_monitor_pids) {
      auto& id = monitorPid.first;
      auto& tasks = monitorPid.second.tasks;
      auto& memUsage = monitorPid.second.memUsage;

      msg::ProcessInfo processInfo;
      processInfo.id = id.pid;
      processInfo.name = id.name;

      // mem info
      {
        msg::MemInfo mem;
        mem.peak = memUsage.VmPeak;
        mem.size = memUsage.VmSize;
        mem.hwm = memUsage.VmHWM;
        mem.rss = memUsage.VmRSS;
        mem.timestamps = timestampsNow;
        processInfo.mem_info = mem;
      }

      for (const auto& task : tasks) {
        msg::ThreadInfo taskInfo;
        taskInfo.id = task->id;
        taskInfo.name = task->name;
        taskInfo.usage = task->usage;
        taskInfo.timestamps = timestampsNow;
        processInfo.thread_infos.push_back(std::move(taskInfo));
      }
      msg.infos.push_back(std::move(processInfo));
    }
    msg.timestamps = timestampsNow;
    s_rpc->cmd("on_process_msg")->msg(msg)->call();
  }
}

static void runServer() {
  initRpcTask();
  using namespace asio_net;
  rpc_config rpc_config;
  rpc_config.rpc = s_rpc;
  rpc_config.max_body_size = MessageMaxByteSize;
  s_rpc_server = std::make_unique<rpc_server>(*s_context, s_argv.s_server_port, std::move(rpc_config));
  s_rpc_server->on_session = [](const std::weak_ptr<rpc_session>& ws) {
    LOGI("device connected");
    ws.lock()->on_close = [] {
      LOGI("device disconnected");
    };
  };
  LOGI("start server: port: %d", s_argv.s_server_port);
  s_rpc_server->start(true);
}

static void updateCpu() {
  s_monitor_cpu->update(true);
  printf("system %s usage: %.2f%%\n", s_monitor_cpu->ave->name.c_str(), s_monitor_cpu->ave->usage);
}

static void updateProcess() {
  for (auto& item : s_monitor_pids) {
    auto& tasks = item.second.tasks;
    auto& memUsage = item.second.memUsage;
    auto memUsageRet = MemMonitor::getUsage(item.first.pid);
    if (!memUsageRet.ok) {
      printf("process exit: name: %-15s, id: %-7" PRIu32 "\n", item.first.name.c_str(), item.first.pid);
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

static void updateProcessChange() {
  std::vector<ProcessKey> alreadyExit;
  for (auto& monitorPid : s_monitor_pids) {
    auto& process = monitorPid.first;
    auto pid = process.pid;
    auto& tasks = monitorPid.second.tasks;

    const auto& taskRet = Utils::getTasksOfPid(pid);
    if (!taskRet.ok) {
      alreadyExit.push_back(process);
      continue;
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

  for (const auto& key : alreadyExit) {
    s_monitor_pids.erase(key);
  }
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
  auto pid_t = (int)strtol(pid.c_str(), nullptr, 10);
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
    updateProcess();
    sendNowCpuInfos();
    updateProcessChange();
    asyncNextUpdate();
  });
}

[[noreturn]] static void monitorCpu() {
  auto& cpu = *s_monitor_cpu;
  for (;;) {
    std::this_thread::sleep_for(std::chrono::milliseconds(s_argv.d_update_interval_ms));
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
        s_argv.all_pids = optarg;
      } break;
      case 'n': {
        s_argv.all_names = optarg;
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

  s_monitor_cpu = std::make_unique<CpuMonitor>();

  if (!s_argv.all_pids.empty()) {
    for (const auto& pidStr : string_utils::Split(s_argv.all_pids, ",", true)) {
      auto pid = std::stoi(pidStr, nullptr, 10);
      LOGD("add pid: %d", pid);
      addMonitorPid(pid);
    }
  }

  if (!s_argv.all_names.empty()) {
    for (const auto& name : string_utils::Split(s_argv.all_names, ",", true)) {
      LOGD("add name: %s", name.c_str());
      addMonitorPidByName(name);
    }
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
