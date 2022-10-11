#include <unistd.h>

#include <algorithm>
#include <memory>
#include <set>

#include "CpuMonitorAll.h"
#include "CpuMsg_generated.h"
#include "RpcMsg.h"
#include "TaskMonitor.h"
#include "Utils.h"
#include "asio.hpp"
#include "log.h"
#include "rpc_server.hpp"

using namespace cpu_monitor;

// main logic
static uint32_t s_server_port = 8088;
static uint32_t s_update_interval_ms = 1000;
static std::vector<PID_t> s_monitor_pids;
static std::unique_ptr<asio::io_context> s_context;
static std::unique_ptr<asio::steady_timer> s_timer_update;

// rpc
static std::unique_ptr<asio_net::rpc_server> s_rpc_server;
static std::shared_ptr<RpcCore::Rpc> s_rpc;
static std::shared_ptr<RpcCore::Dispose> s_dispose;

// cpu monitor
static std::unique_ptr<CpuMonitorAll> s_monitor_cpu;
static const auto s_total_time_impl = [] {
  return s_monitor_cpu->ave->totalTime;
};
static std::set<std::unique_ptr<TaskMonitor>> s_monitor_tasks;

static void initRpcTask() {
  s_rpc->subscribe("add_pid", [] {

  });
  s_rpc->subscribe("del_pid", [] {

  });
  s_rpc->subscribe("get_added_pids", [] {

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
    for (const auto& item : s_monitor_cpu->cores) {
      auto info = std::make_unique<msg::CpuInfoT>();
      info->name = item->stat().name;
      info->usage = item->usage;
      msg.msg.infos.push_back(std::move(info));
    }
    s_rpc->createRequest()->cmd("on_cpm_msg")->msg(msg)->noRsp()->call();
  }

  // progress info
  {
    RpcMsg<msg::CpuMsgT> msg;
    for (const auto& item : s_monitor_tasks) {
      auto info = std::make_unique<msg::CpuInfoT>();
      info->name = item->stat().name;
      info->usage = item->usage;
      msg.msg.infos.push_back(std::move(info));
    }
    s_rpc->createRequest()->cmd("on_progress_msg")->msg(msg)->noRsp()->call();
  }
}

static void runServer() {
  using namespace asio_net;
  s_rpc_server = std::make_unique<rpc_server>(*s_context, 8088, 1024 * 1024 * 1);
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
    s_dispose = std::make_shared<RpcCore::Dispose>();
    initRpcTask();
  };

  LOGI("start server: port: %d", s_server_port);
  server->start(true);
}

static void updateCpu() {
  s_monitor_cpu->update(true);
  printf("system %s usage: %.2f%%\n", s_monitor_cpu->ave->stat().name, s_monitor_cpu->ave->usage);
}

static bool updatePid(PID_t pid) {
  for (auto& item : s_monitor_tasks) {
    bool ok = item->update();
    if (ok) {
      printf("name: %s, id: %lu, usage: %.2f%%\n", item->stat().name.c_str(), item->stat().id, item->usage * 100);
    } else {
      printf("thread exit: name: %s, id: %lu\n", item->stat().name.c_str(), item->stat().id);
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
  const auto isTaskAlive = [&tasksNow](TaskId_t taskId) {
    return std::find(tasksNow.cbegin(), tasksNow.cend(), taskId) != tasksNow.cend();
  };
  for (auto iter = s_monitor_tasks.begin(); iter != s_monitor_tasks.cend();) {
    // delete not cared task
    if (!isTaskAlive(iter->get()->stat().id)) {
      iter = s_monitor_tasks.erase(iter);
    } else {
      ++iter;
    }
  }

  // 添加新增的线程
  const auto isNewTask = [](TaskId_t taskId) {
    return std::find_if(s_monitor_tasks.cbegin(), s_monitor_tasks.cend(), [taskId](const std::unique_ptr<TaskMonitor>& monitor) {
             return monitor->stat().id == taskId;
           }) == s_monitor_tasks.cend();
  };
  for (const auto& item : tasksNow) {
    if (isNewTask(item)) {
      s_monitor_tasks.insert(std::make_unique<TaskMonitor>(Utils::makeTaskStatPath(pid, item), s_total_time_impl));
    }
  }

  return true;
}

static bool initMonitor() {
  auto pid = Utils::getFirstPidByName("node");
  auto ret = Utils::getTasksOfPid(pid);
  LOGI("get task info: pid: %u, ok: %d, num: %zu", pid, ret.ok, ret.ids.size());
  if (!ret.ok) {
    LOGE("pid not found: %u", pid);
    return false;
  }

  for (auto tid : ret.ids) {
    LOGI("thread id: %d", tid);
    s_monitor_tasks.insert(std::make_unique<TaskMonitor>(Utils::makeTaskStatPath(pid, tid), s_total_time_impl));
  }

  // add to monitor
  s_monitor_pids.push_back(pid);

  return true;
}

static void asyncNextUpdate() {
  s_timer_update->expires_after(std::chrono::milliseconds(s_update_interval_ms));
  s_timer_update->async_wait([](asio::error_code ec) {
    updateCpu();
    for (const auto& pid : s_monitor_pids) {
      updatePid(pid);
    }
    sendNowCpuInfos();
    asyncNextUpdate();
  });
}

static void initApp() {
  s_monitor_cpu = std::make_unique<CpuMonitorAll>();

  s_context = std::make_unique<asio::io_context>();
  s_timer_update = std::make_unique<asio::steady_timer>(*s_context);

  asyncNextUpdate();
}

static void runApp() {
  initApp();
  initMonitor();
  runServer();
}

static void showHelp() {
  printf(R"(Usage:
-p : 指定端口号 默认8088
)");
}

int main(int argc, char** argv) {
  int ret;
  while ((ret = getopt(argc, argv, "p:")) != -1) {
    switch (ret) {  // NOLINT
      case 'p': {
        s_server_port = std::stoi(optarg, nullptr, 10);
        LOGD("s_server_port: %d", s_server_port);
      } break;
      default:
        showHelp();
        break;
    }
  }
  runApp();
  return 0;
}
