#include "Home.h"

#include "App.h"
#include "CpuMsg_generated.h"
#include "ProgressMsg_generated.h"
#include "RpcMsg.h"
#include "Types.h"
#include "imgui.h"
#include "implot.h"
#include "log.h"
#include "rpc_client.hpp"
#include "uidef.h"

using namespace cpu_monitor;

// rpc
static std::unique_ptr<asio_net::rpc_client> s_rpc_client;
static std::shared_ptr<RpcCore::Rpc> s_rpc;
static std::shared_ptr<RpcCore::Dispose> s_dispose;

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

static std::unique_ptr<asio::steady_timer> s_timer_connect;

static void initRpcTask() {
  s_rpc->subscribe<RpcMsg<msg::CpuMsgT>>("on_cpu_msg", [](RpcMsg<msg::CpuMsgT> msg) {
    s_msg_cpus.push_back(std::move(msg.msg));
  });

  s_rpc->subscribe<RpcMsg<msg::ProgressMsgT>>("on_progress_msg", [](RpcMsg<msg::ProgressMsgT> msg) {
    auto& progressMsg = msg.msg;
    for (auto& pInfo : progressMsg.infos) {
      auto& progressInfos = s_msg_pids[{(PID_t)pInfo->id, pInfo->name}];
      for (auto& item : pInfo->infos) {
        progressInfos[item->id].push_back(std::move(item));
      }
    }
  });
}

static void startAutoConnect();

static void connectServer() {
  s_rpc_client = std::make_unique<asio_net::rpc_client>(App::instance()->context());
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
  client->on_open_failed = [] {
    startAutoConnect();
  };
  LOGI("try open...");
  client->open("10.238.21.156", "8088");
}

static void startAutoConnect() {
  s_timer_connect->expires_after(std::chrono::seconds(1));
  s_timer_connect->async_wait([](asio::error_code ec) {
    if (!s_rpc) {
      connectServer();
    }
  });
}

namespace ui {
namespace flag {
bool showCpuAve = true;
bool showCpuCores = true;
}  // namespace flag
}  // namespace ui

void Home::onDraw() const {
  using namespace ImGui;

  ImGui::Begin("MainWindow", nullptr, windowFlags_);  // NOLINT
  PushItemWidth(ui::ITEM_WIDTH);

  Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GetIO().Framerate, GetIO().Framerate);

  if (Button("connect")) {
    if (s_rpc) {
      LOGI("已连接");
    } else {
      connectServer();
    }
  }

  ImGui::SameLine();
  ImGui::Checkbox("Show Ave", &ui::flag::showCpuAve);

  ImGui::SameLine();
  ImGui::Checkbox("Show Cores", &ui::flag::showCpuCores);

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
          LOGD("add name: %s", name.c_str());
          if (s_rpc) {
            s_rpc->createRequest()
                ->cmd("add_name")
                ->msg(RpcCore::String(name))
                ->rsp<RpcCore::String>([](const RpcCore::String& msg) {
                  LOGD("add name rsp: %s", msg.c_str());
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
          LOGD("del name: %s", name.c_str());
          if (s_rpc) {
            s_rpc->createRequest()
                ->cmd("del_name")
                ->msg(RpcCore::String(name))
                ->rsp<RpcCore::String>([](const RpcCore::String& msg) {
                  LOGD("del name rsp: %s", msg.c_str());
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
          LOGD("add pid: %s", pid.c_str());
          if (s_rpc) {
            s_rpc->createRequest()
                ->cmd("add_pid")
                ->msg(RpcCore::String(pid))
                ->rsp<RpcCore::String>([](const RpcCore::String& msg) {
                  LOGD("add pid rsp: %s", msg.c_str());
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
          LOGD("del pid: %s", pid.c_str());
          if (s_rpc) {
            s_rpc->createRequest()
                ->cmd("del_pid")
                ->msg(RpcCore::String(pid))
                ->rsp<RpcCore::String>([](const RpcCore::String& msg) {
                  LOGD("del pid rsp: %s", msg.c_str());
                })
                ->call();
          }
        }
      }
    }
  }

  // ave
  if (ui::flag::showCpuAve) {
    ImPlot::BeginPlot("Cpu Ave Usages (%/sec)");
    int axisFlags = ImPlotAxisFlags_NoLabel;
    const int axisXMin = 10;
    ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

    if (s_msg_cpus.size() <= axisXMin) {
      axisFlags |= ImPlotAxisFlags_Lock;
    } else {
      axisFlags |= ImPlotAxisFlags_AutoFit;
    }

    ImPlot::SetupAxes("Time(sec)", "Usages(%)", axisFlags, ImPlotAxisFlags_Lock);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_NoButtons);
    if (!s_msg_cpus.empty()) {
      ImPlot::PlotLineG(
          s_msg_cpus.front().ave->name.c_str(),
          (ImPlotGetter)[](int idx, void* user_data) {
            return ImPlotPoint{(double)idx, s_msg_cpus[idx].ave->usage};
          },
          nullptr, (int)s_msg_cpus.size());

      // shade
      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
      ImPlot::PlotShadedG(
          s_msg_cpus.front().ave->name.c_str(),
          (ImPlotGetter)[](int idx, void* user_data) {
            return ImPlotPoint{(double)idx, 0};
          },
          nullptr,
          (ImPlotGetter)[](int idx, void* user_data) {
            return ImPlotPoint{(double)idx, s_msg_cpus[idx].ave->usage};
          },
          nullptr, (int)s_msg_cpus.size(), 0);
      ImPlot::PopStyleVar();
    }
    ImPlot::EndPlot();
  }

  // cores
  if (ui::flag::showCpuCores) {
    ImPlot::BeginPlot("Cpu Cores Usages (%/sec)");
    int axisFlags = ImPlotAxisFlags_NoLabel;
    const int axisXMin = 10;
    ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

    if (s_msg_cpus.size() <= axisXMin) {
      axisFlags |= ImPlotAxisFlags_Lock;
    } else {
      axisFlags |= ImPlotAxisFlags_AutoFit;
    }

    ImPlot::SetupAxes("Time(sec)", "Usages(%)", axisFlags, axisFlags);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Outside);
    if (!s_msg_cpus.empty()) {
      for (int i = 0; i < s_msg_cpus.front().cores.size(); ++i) {
        static int indexNow;
        indexNow = i;
        ImPlot::PlotLineG(
            s_msg_cpus.front().cores[indexNow]->name.c_str(),
            (ImPlotGetter)[](int idx, void* user_data) {
              return ImPlotPoint{(double)idx, s_msg_cpus[idx].cores[indexNow]->usage};
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

      auto plotName = "pid: " + std::to_string(progressKey.pid) + ": " + progressKey.name;
      ImPlot::BeginPlot(plotName.c_str());
      int axisFlags = ImPlotAxisFlags_NoLabel;
      const int axisXMin = 10;
      ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

      if (s_msg_cpus.size() <= axisXMin) {
        axisFlags |= ImPlotAxisFlags_Lock;
      } else {
        axisFlags |= ImPlotAxisFlags_AutoFit;
      }

      ImPlot::SetupAxes("Time(sec)", "Usages(%)", axisFlags, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_Lock);
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_None);
      if (!threadInfoTable.empty()) {
        for (auto& item : threadInfoTable) {
          const static ThreadInfosType* threadInfos;
          threadInfos = &(item.second);

          {
            static int indexNow;
            indexNow = 0;

            auto& threadInfo = threadInfos[indexNow];
            auto labelName = std::string("tid: ") + std::to_string(threadInfo.front()->id) + " name: " + threadInfo.front()->name;
            ImPlot::PlotLineG(
                labelName.c_str(),
                (ImPlotGetter)[](int idx, void* user_data) {
                  return ImPlotPoint{(double)idx, (*threadInfos)[indexNow]->usage};
                },
                nullptr, (int)threadInfos->size());
          }
        }
      }
      ImPlot::EndPlot();
    }
  }

  // plot demo
  if (0) {
    static bool open = true;
    ImPlot::ShowDemoWindow(&open);
  }
  ImGui::End();
}

void Home::initGUI() {
  windowFlags_ |= ImGuiWindowFlags_NoMove;
  windowFlags_ |= ImGuiWindowFlags_NoResize;
  windowFlags_ |= ImGuiWindowFlags_NoCollapse;
  windowFlags_ |= ImGuiWindowFlags_NoTitleBar;

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
