#include "Home.h"

#include "App.h"
#include "Common.h"
#include "CpuMsg_generated.h"
#include "ProgressMsg_generated.h"
#include "RpcMsg.h"
#include "Types.h"
#include "defer.h"
#include "imgui.h"
#include "implot.h"
#include "log.h"
#include "rpc_client.hpp"

using namespace cpu_monitor;

namespace ui {
namespace flag {
bool useADB = true;
bool showCpuAve = true;
bool showCpuCores = true;
std::string serverAddr = "10.238.21.156";  // NOLINT
std::string serverPort = "8088";           // NOLINT
}  // namespace flag
}  // namespace ui

// rpc
static std::unique_ptr<asio_net::rpc_client> s_rpc_client;
static std::shared_ptr<RpcCore::Rpc> s_rpc;

static std::vector<msg::CpuMsgT> s_msg_cpus;

struct ProgressKey {
  PID_t pid;
  std::string name;

  friend inline bool operator<(const ProgressKey& a, const ProgressKey& b) {
    return a.pid < b.pid;
  }
};

using ThreadInfosType = std::vector<std::unique_ptr<cpu_monitor::msg::ThreadInfoT>>;
using ThreadInfoTable = std::map<TaskId_t, ThreadInfosType>;
static std::map<ProgressKey, ThreadInfoTable> s_msg_pids;
static std::map<PID_t, uint> s_pid_current_thread_num;

static std::unique_ptr<asio::steady_timer> s_timer_connect;

static void initRpcTask() {
  s_rpc->subscribe("on_cpu_msg", [](RpcMsg<msg::CpuMsgT> msg) {
    for (const auto& item : msg->cores) {
      item->timestamps = msg->timestamps;
    }
    s_msg_cpus.push_back(std::move(msg.msg));
  });

  s_rpc->subscribe("on_progress_msg", [](RpcMsg<msg::ProgressMsgT> msg) {
    auto& progressMsg = msg.msg;
    for (auto& pInfo : progressMsg.infos) {
      auto& progressInfos = s_msg_pids[{(PID_t)pInfo->id, pInfo->name}];
      s_pid_current_thread_num[pInfo->id] = pInfo->infos.size();
      for (auto& item : pInfo->infos) {
        item->timestamps = progressMsg.timestamps;
        progressInfos[item->id].push_back(std::move(item));
      }
    }
  });
}

static void startAutoConnect();

static void connectServer() {
  s_rpc_client = std::make_unique<asio_net::rpc_client>(App::instance()->context(), MessageMaxByteSize);
  auto& client = s_rpc_client;
  client->on_open = [](std::shared_ptr<RpcCore::Rpc> rpc) {
    LOGI("on_open");
    s_rpc = std::move(rpc);
    initRpcTask();
  };
  client->on_close = [] {
    s_rpc = nullptr;
    startAutoConnect();
  };
  client->on_open_failed = [](const std::error_code& ec) {
    LOGW("on_open_failed: %d, %s", ec.value(), ec.message().c_str());
    startAutoConnect();
  };
  LOGI("try open...");
  if (ui::flag::useADB) {
    system("adb forward tcp:8088 tcp:8088");
    client->open("localhost", "8088");
  } else {
    client->open(ui::flag::serverAddr, ui::flag::serverPort);
  }
}

static void startAutoConnect() {
  s_timer_connect->expires_after(std::chrono::seconds(1));
  s_timer_connect->async_wait([](asio::error_code ec) {
    if (!s_rpc) {
      connectServer();
    }
  });
}

void Home::onDraw() {
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

  ImGui::SameLine();
  if (ImGui::Button("GetPids")) {
    if (s_rpc) {
      s_rpc->cmd("get_added_pids")
          ->rsp([](RpcMsg<msg::ProgressMsgT> msg) {
            LOGI("get_added_pids rsp:");
            for (const auto& item : msg->infos) {
              LOGI("pid: %llu, name: %s", item->id, item->name.c_str());
            }
          })
          ->call();
    }
  }

  ImGui::SameLine();
  ImGui::Checkbox("ADB", &ui::flag::useADB);

  if (!ui::flag::useADB) {
    ImGui::PushItemWidth(120);
    {
      ui::flag::serverAddr.reserve(64);
      ImGui::SameLine();
      ImGui::InputText("ip##input_server_ip", (char*)ui::flag::serverAddr.data(), ui::flag::serverAddr.capacity());
    }
    {
      ui::flag::serverPort.reserve(64);
      ImGui::SameLine();
      ImGui::InputText("port##input_server_port", (char*)ui::flag::serverPort.data(), ui::flag::serverPort.capacity());
    }
    ImGui::PopItemWidth();
  }

  ImGui::SameLine();
  ImGui::Checkbox("Show Ave", &ui::flag::showCpuAve);

  ImGui::SameLine();
  ImGui::Checkbox("Show Cores", &ui::flag::showCpuCores);

  static auto calcTimestampsFromStart = [](uint64_t timestamps) -> double {
    return double(timestamps - s_msg_cpus.front().timestamps) / 1000;
  };

  {
    // NAME
    {
      ImGui::SameLine();
      static std::string name = "cpu_monitor";
      name.resize(32);
      ImGui::PushItemWidth(120);
      ImGui::Text("Name:");
      ImGui::SameLine();
      ImGui::InputText("##add_name", (char*)name.data(), name.size());
      ImGui::PopItemWidth();
      ImGui::SameLine();
      if (ImGui::Button("Add##add_name_btn")) {
        if (name[0] == 0) {
          LOGW("name empty");
        } else {
          LOGI("add name: %s", name.c_str());
          if (s_rpc) {
            s_rpc->cmd("add_name")
                ->msg(RpcCore::String(name.c_str()))  // NOLINT
                ->rsp([](const RpcCore::String& msg) {
                  LOGI("add name rsp: %s", msg.c_str());
                })
                ->call();
          }
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Del##del_name_btn")) {
        if (name[0] == 0) {
          LOGW("name empty");
        } else {
          LOGI("del name: %s", name.c_str());
          if (s_rpc) {
            s_rpc->cmd("del_name")
                ->msg(RpcCore::String(name.c_str()))  // NOLINT
                ->rsp([](const RpcCore::String& msg) {
                  LOGI("del name rsp: %s", msg.c_str());
                })
                ->call();
          }
        }
      }
    }
    // PID
    {
      ImGui::SameLine();
      static std::string pid;
      pid.resize(60);
      ImGui::PushItemWidth(80);
      ImGui::Text("PID:");
      ImGui::SameLine();
      ImGui::InputText("##add_pid", (char*)pid.data(), pid.size(), ImGuiInputTextFlags_CharsDecimal);
      ImGui::PopItemWidth();
      ImGui::SameLine();
      if (ImGui::Button("Add##add_pid_btn")) {
        if (pid[0] == 0) {
          LOGW("pid empty");
        } else {
          LOGI("add pid: %s", pid.c_str());
          if (s_rpc) {
            s_rpc->cmd("add_pid")
                ->msg(RpcCore::String(pid.c_str()))  // NOLINT
                ->rsp([](const RpcCore::String& msg) {
                  LOGI("add pid rsp: %s", msg.c_str());
                })
                ->call();
          }
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Del##del_pid_btn")) {
        if (pid[0] == 0) {
          LOGW("pid empty");
        } else {
          LOGI("del pid: %s", pid.c_str());
          if (s_rpc) {
            s_rpc->cmd("del_pid")
                ->msg(RpcCore::String(pid.c_str()))  // NOLINT
                ->rsp([](const RpcCore::String& msg) {
                  LOGI("del pid rsp: %s", msg.c_str());
                })
                ->call();
          }
        }
      }
    }
  }

  // ave
  if (ui::flag::showCpuAve && ImPlot::BeginPlot("Cpu Ave Usages (%/sec)")) {
    const int axisXMin = 10;
    ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

    ImPlot::SetupAxes("Time(sec)", "Usages(%)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_Lock);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
    if (!s_msg_cpus.empty()) {
      ImPlot::PlotLineG(
          s_msg_cpus.front().ave->name.c_str(),
          (ImPlotGetter)[](int idx, void* user_data) {
            auto& info = s_msg_cpus[idx];
            return ImPlotPoint{calcTimestampsFromStart(info.timestamps), info.ave->usage};
          },
          nullptr, (int)s_msg_cpus.size());

      // shade
      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
      ImPlot::PlotShadedG(
          s_msg_cpus.front().ave->name.c_str(),
          (ImPlotGetter)[](int idx, void* user_data) {
            auto& info = s_msg_cpus[idx];
            return ImPlotPoint{calcTimestampsFromStart(info.timestamps), 0};
          },
          nullptr,
          (ImPlotGetter)[](int idx, void* user_data) {
            auto& info = s_msg_cpus[idx];
            return ImPlotPoint{calcTimestampsFromStart(info.timestamps), info.ave->usage};
          },
          nullptr, (int)s_msg_cpus.size(), 0);
      ImPlot::PopStyleVar();
    }
    ImPlot::EndPlot();
  }

  // cores
  std::string coreInfo = []() -> std::string {
    if (s_msg_cpus.empty()) return "";
    return " cores: " + std::to_string(s_msg_cpus.front().cores.size());
  }();
  if (ui::flag::showCpuCores && ImPlot::BeginPlot(("Cpu Cores Usages (%/sec)" + coreInfo).c_str())) {
    const int axisXMin = 10;
    ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

    int axisFlags = ImPlotAxisFlags_NoLabel;
    axisFlags |= ImPlotAxisFlags_AutoFit;

    ImPlot::SetupAxes("Time(sec)", "Usages(%)", axisFlags, axisFlags);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Outside);
    if (!s_msg_cpus.empty()) {
      for (int i = 0; i < s_msg_cpus.front().cores.size(); ++i) {
        static int indexNow;
        indexNow = i;
        ImPlot::PlotLineG(
            s_msg_cpus.front().cores[indexNow]->name.c_str(),
            (ImPlotGetter)[](int idx, void* user_data) {
              auto& info = s_msg_cpus[idx].cores[indexNow];
              return ImPlotPoint{calcTimestampsFromStart(info->timestamps), info->usage};
            },
            nullptr, (int)s_msg_cpus.size());
      }
    }
    ImPlot::EndPlot();
  }

  // pids
  {
    for (const auto& msgPid : s_msg_pids) {
      auto& progressKey = msgPid.first;
      auto& threadInfoTable = msgPid.second;

      auto plotName = "pid: " + std::to_string(progressKey.pid) + " name: " + progressKey.name +
                      " threads: " + std::to_string(s_pid_current_thread_num[progressKey.pid]);
      if (!ImPlot::BeginPlot(plotName.c_str())) {
        break;
      }
      const int axisXMin = 10;
      ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

      int axisFlags = ImPlotAxisFlags_NoLabel;
      axisFlags |= ImPlotAxisFlags_AutoFit;

      ImPlot::SetupAxes("Time(sec)", "Usages(%)", axisFlags, ImPlotAxisFlags_AutoFit);
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_None);
      if (!threadInfoTable.empty()) {
        int indexNow = 0;
        for (auto& item : threadInfoTable) {
          defer {
            ++indexNow;
          };
          const static ThreadInfosType* threadInfos;
          threadInfos = &(item.second);

          auto labelName = std::string("tid: ") + std::to_string(threadInfos->front()->id) + " name: " + threadInfos->front()->name;
          ImPlot::PlotLineG(
              labelName.c_str(),
              (ImPlotGetter)[](int idx, void* user_data) {
                auto& info = (*threadInfos)[idx];
                return ImPlotPoint{calcTimestampsFromStart(info->timestamps), info->usage};
              },
              nullptr, (int)threadInfos->size());
        }
      }
      ImPlot::EndPlot();
    }
  }

  // imgui demo
#if 0
  {
    static bool open = true;
    ImGui::ShowDemoWindow(&open);
  }
#endif

  // plot demo
#if 0
  {
    static bool open = true;
    ImPlot::ShowDemoWindow(&open);
  }
#endif
}

void Home::initGUI() {
  ImPlot::CreateContext();
}

Home::Home() {
  initGUI();

  s_timer_connect = std::make_unique<asio::steady_timer>(App::instance()->context());
  startAutoConnect();
}

Home::~Home() {
  ImPlot::DestroyContext();
}
