#include <unistd.h>

#include <algorithm>
#include <memory>
#include <set>

#include "Common.h"
#include "CpuMonitorAll.h"
#include "CpuMsg_generated.h"
#include "ProgressMsg_generated.h"
#include "RpcMsg.h"
#include "TaskMonitor.h"
#include "Utils.h"
#include "asio.hpp"
#include "log.h"
#include "rpc_server.hpp"
#include "string_utils.h"

using namespace cpu_monitor;

// main logic
static uint32_t s_server_port = 8088;
static uint32_t s_update_interval_ms = 1000;
static std::unique_ptr<asio::io_context> s_context;
static std::unique_ptr<asio::steady_timer> s_timer_update;

// rpc
static std::unique_ptr<asio_net::rpc_server> s_rpc_server;
static std::shared_ptr<RpcCore::Rpc> s_rpc;

// cpu monitor
static std::unique_ptr<CpuMonitorAll> s_monitor_cpu;
static const auto s_total_time_impl = [] {
  return s_monitor_cpu->ave->totalTime / s_monitor_cpu->cores.size();
};

using MonitorTasks = std::vector<std::unique_ptr<TaskMonitor>>;
struct ProgressKey {
  PID_t pid;
  std::string name;

  friend inline bool operator<(const ProgressKey& a, const ProgressKey& b) {
    return a.pid < b.pid;
  }
};
using MonitorPids = std::map<ProgressKey, MonitorTasks>;
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

  // cpu info
  {
    RpcMsg<msg::CpuMsgT> msg;
    // ave
    {
      auto info = std::make_unique<msg::CpuInfoT>();
      info->name = s_monitor_cpu->ave->stat().name;
      info->usage = s_monitor_cpu->ave->usage;
      msg->ave = std::move(info);
    }
    // cores
    for (const auto& core : s_monitor_cpu->cores) {
      auto info = std::make_unique<msg::CpuInfoT>();
      info->name = core->stat().name;
      info->usage = core->usage;
      msg->cores.push_back(std::move(info));
    }
    s_rpc->cmd("on_cpu_msg")->msg(msg)->call();
  }

  // progress info
  {
    RpcMsg<msg::ProgressMsgT> msg;
    for (const auto& monitorPid : s_monitor_pids) {
      auto& id = monitorPid.first;
      auto& tasks = monitorPid.second;

      auto progressInfo = std::make_unique<msg::ProgressInfoT>();
      progressInfo->id = id.pid;
      progressInfo->name = id.name;
      for (const auto& task : tasks) {
        auto taskInfo = std::make_unique<msg::ThreadInfoT>();
        taskInfo->id = task->stat().id;
        taskInfo->name = task->stat().name;
        taskInfo->usage = task->usage;
        progressInfo->infos.push_back(std::move(taskInfo));
      }
      msg->infos.push_back(std::move(progressInfo));
    }
    s_rpc->cmd("on_progress_msg")->msg(msg)->call();
  }
}

static void runServer() {
  using namespace asio_net;
  s_rpc_server = std::make_unique<rpc_server>(*s_context, 8088, MessageMaxByteSize);
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

  LOGI("start server: port: %d", s_server_port);
  server->start(true);
}

static void updateCpu() {
  s_monitor_cpu->update(true);
  printf("system %s usage: %.2f%%\n", s_monitor_cpu->ave->stat().name, s_monitor_cpu->ave->usage);
}

static bool updateProgress() {
  for (auto& monitorPid : s_monitor_pids) {
    auto& id = monitorPid.first;
    auto pid = id.pid;
    auto& tasks = monitorPid.second;

    for (auto& task : tasks) {
      bool ok = task->update();
      if (ok) {
        printf("name: %-15s, id: %-7" PRIu64 ", usage: %.2f%%\n", task->stat().name.c_str(), task->stat().id, task->usage);
      } else {
        printf("thread exit: name: %s, id: %" PRIu64 "\n", task->stat().name.c_str(), task->stat().id);
      }
    }
    printf("\n");

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
      if (!isTaskAlive(iter->get()->stat().id)) {
        iter = tasks.erase(iter);
      } else {
        ++iter;
      }
    }

    // 添加新增的线程
    const auto isNewTask = [&](TaskId_t taskId) {
      return std::find_if(tasks.cbegin(), tasks.cend(), [taskId](const std::unique_ptr<TaskMonitor>& monitor) {
               return monitor->stat().id == taskId;
             }) == tasks.cend();
    };
    for (const auto& tid : tasksNow) {
      if (isNewTask(tid)) {
        tasks.push_back(std::make_unique<TaskMonitor>(pid, tid, s_total_time_impl));
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
    monitorTask.push_back(std::make_unique<TaskMonitor>(pid, tid, s_total_time_impl));
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
  auto pid = Utils::getFirstPidByName(name);
  return addMonitorPid(pid);
}

static void asyncNextUpdate() {
  s_timer_update->expires_after(std::chrono::milliseconds(s_update_interval_ms));
  s_timer_update->async_wait([](asio::error_code ec) {
    updateCpu();
    updateProgress();
    sendNowCpuInfos();
    asyncNextUpdate();
  });
}

[[noreturn]] static void monitorCpu() {
  CpuMonitorAll cpu;
  for (;;) {
    sleep(1);
    cpu.update();
    printf("%s usage: %.2f%%\n", cpu.ave->stat().name, cpu.ave->usage);
    for (auto& item : cpu.cores) {
      printf("%s usage: %.2f%%\n", item->stat().name, item->usage);
    }
    printf("\n");
  }
}

static void initApp() {
  s_monitor_cpu = std::make_unique<CpuMonitorAll>();

  s_context = std::make_unique<asio::io_context>();
  s_timer_update = std::make_unique<asio::steady_timer>(*s_context);

  asyncNextUpdate();
}

static void runApp() {
  s_context->run();
}

static void showHelp() {
  printf(R"(Usage:
-s : 以服务方式启动（配合GUI） 并指定端口号
-c : 监控所有CPU核
-p : 指定监控的PID 半角逗号分隔
-n : 指定监控进程名 半角逗号分隔
)");
}
int main(int argc, char** argv) {
  if (argc < 2) {
    showHelp();
    return 0;
  }

  int ret;
  while ((ret = getopt(argc, argv, "s:c::p:n:")) != -1) {
    switch (ret) {
      case 's': {
        s_server_port = std::stoul(optarg, nullptr, 10);
        LOGD("s_server_port: %u", s_server_port);
        initApp();
        runServer();
      } break;
      case 'c':
        monitorCpu();
      case 'p': {
        auto& allPids = optarg;
        for (const auto& pidStr : string_utils::Split(allPids, ",", true)) {
          auto pid = std::stoul(pidStr, nullptr, 10);
          LOGD("add pid: %lu", pid);
          addMonitorPid(pid);
        }
        initApp();
        runApp();
      } break;
      case 'n': {
        auto& allNames = optarg;
        for (const auto& name : string_utils::Split(allNames, ",", true)) {
          LOGD("add name: %s", name.c_str());
          addMonitorPidByName(name);
        }
        initApp();
        runApp();
      } break;
      default:
        showHelp();
        break;
    }
  }
  return 0;
}
