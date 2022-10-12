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

static std::unique_ptr<asio::steady_timer> s_timer_connect;

static void initRpcTask() {
  s_rpc->subscribe<RpcMsg<msg::CpuMsgT>>("on_cpm_msg", [](RpcMsg<msg::CpuMsgT> msg) {
    s_msg_cpus.push_back(std::move(msg.msg));
  });

  s_rpc->subscribe<RpcMsg<msg::ProgressMsgT>>("on_progress_msg", [](RpcMsg<msg::ProgressMsgT> msg) {
    s_msg_pids[1].push_back(std::move(msg.msg.infos));
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

  // plot demo
  if (0) {
    bool open = true;
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
