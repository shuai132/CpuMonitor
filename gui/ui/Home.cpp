#include "Home.h"

#include "App.h"
#include "CpuMsg_generated.h"
#include "ProgressMsg_generated.h"
#include "RpcMsg.h"
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
static std::map<uint32_t, std::vector<decltype(msg::ProgressMsgT::infos)>> s_msg_pids;

static void initRpcTask() {
  s_rpc->subscribe<RpcMsg<msg::CpuMsgT>>("on_cpm_msg", [](RpcMsg<msg::CpuMsgT> msg) {
    s_msg_cpus.push_back(std::move(msg.msg));
  });

  s_rpc->subscribe<RpcMsg<msg::ProgressMsgT>>("on_progress_msg", [](RpcMsg<msg::ProgressMsgT> msg) {
    s_msg_pids[1].push_back(std::move(msg.msg.infos));
  });
}

static void connectServer() {
  s_rpc_client = std::make_unique<asio_net::rpc_client>(App::instance()->context());
  s_rpc_client->on_open = [](std::shared_ptr<RpcCore::Rpc> rpc) {
    LOGI("on_open");
    s_rpc = std::move(rpc);
    initRpcTask();
  };
  LOGI("try open...");
  s_rpc_client->open("10.238.21.156", "8088");
}

void Home::onDraw() const {
  using namespace ImGui;

  ImGui::Begin("MainWindow", nullptr, windowFlags_);  // NOLINT
  ImGui::SetWindowPos({0, 0});
  ImGui::SetWindowSize({ui::MainWindowWidth, ui::MainWindowHeight});
  PushItemWidth(ui::ITEM_WIDTH);

  Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GetIO().Framerate, GetIO().Framerate);

  if (Button("connect")) {
    if (s_rpc) {
      LOGI("已连接");
    } else {
      connectServer();
    }
  }

  // ave
  {
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
    }
    ImPlot::EndPlot();
  }

  // cores
  {
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
  //  bool open = true;
  //  ImPlot::ShowDemoWindow(&open);
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
}

Home::~Home() {
  ImPlot::DestroyContext();
}
