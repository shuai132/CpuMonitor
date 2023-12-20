#include "Home.h"

#include <utility>

#include "App.h"
#include "Common.h"
#include "Types.h"
#include "asio_net/rpc_client.hpp"
#include "file_utils.h"
#include "imgui.h"
#include "implot.h"
#include "log.h"

// msg data
#include "MsgData.hpp"

static MsgData s_msg;
auto& s_msg_cpus = s_msg.msg_cpus;
auto& s_msg_pids = s_msg.msg_pids;
auto& s_pid_current_thread_num = s_msg.pid_current_thread_num;

using namespace cpu_monitor;

namespace ui {
namespace flag {
bool useLocal = true;
bool showCpuAve = true;
bool showCpuCores = true;
bool showCpu = true;
bool showMem = true;
bool showTest = false;
bool showLoadData = false;
bool showSettings = false;
std::string serverAddr = "10.238.21.156";  // NOLINT
std::string serverPort = "8088";           // NOLINT
}  // namespace flag
}  // namespace ui

// rpc
static std::unique_ptr<asio_net::rpc_client> s_rpc_client;
static std::shared_ptr<rpc_core::rpc> s_rpc;

static void initRpc() {
  s_rpc = rpc_core::rpc::create();
  s_rpc->subscribe("on_cpu_msg", [](msg::CpuMsg msg) {
    if (ui::flag::showTest) return;
    if (ui::flag::showLoadData) return;
    s_msg.process(std::move(msg));
  });

  s_rpc->subscribe("on_process_msg", [](msg::ProcessMsg msg) {
    if (ui::flag::showTest) return;
    if (ui::flag::showLoadData) return;
    s_msg.process(std::move(msg));
  });
}

static void initClient() {
  using namespace asio_net;
  rpc_config rpc_config;
  rpc_config.rpc = s_rpc;
  rpc_config.max_body_size = MessageMaxByteSize;
  s_rpc_client = std::make_unique<rpc_client>(App::instance()->context(), rpc_config);
  auto& client = s_rpc_client;
  client->on_open = [](const std::shared_ptr<rpc_core::rpc>& rpc) {
    LOGI("on_open");
  };
  client->on_open_failed = [](const std::error_code& ec) {
    LOGI("on_open_failed: %s", ec.message().c_str());
  };
  client->on_close = [] {
    LOGI("on_close");
  };
  client->set_reconnect(1000);
}

static void connectServer() {
  auto& client = s_rpc_client;
  if (ui::flag::useLocal) {
    client->open("localhost", std::strtol(ui::flag::serverPort.c_str(), nullptr, 10));
    LOGI("try open usb: localhost:%s", ui::flag::serverPort.c_str());
  } else {
    client->open(ui::flag::serverAddr, std::strtol(ui::flag::serverPort.c_str(), nullptr, 10));
    LOGI("try open tcp: %s:%s", ui::flag::serverAddr.c_str(), ui::flag::serverPort.c_str());
  }
}

static void checkIpChange() {
  static bool useLocal = false;
  static std::string serverAddr;
  static std::string serverPort;
  if (useLocal != ui::flag::useLocal || serverAddr != ui::flag::serverAddr || serverPort != ui::flag::serverPort) {
    useLocal = ui::flag::useLocal;
    serverAddr = ui::flag::serverAddr;
    serverPort = ui::flag::serverPort;
    connectServer();
  }
}

void Home::onDraw() {
  ImGui::Checkbox("LOCAL", &ui::flag::useLocal);

  // server ip
  if (!ui::flag::useLocal) {
    ui::flag::serverAddr.reserve(64);
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    ImGui::InputText("ip##input_server_ip", (char*)ui::flag::serverAddr.data(), ui::flag::serverAddr.capacity());
    ImGui::PopItemWidth();
  }

  // port
  {
    ui::flag::serverPort.reserve(16);
    ImGui::SameLine();
    ImGui::PushItemWidth(40);
    ImGui::InputText("port##input_server_port", (char*)ui::flag::serverPort.data(), ui::flag::serverPort.capacity());
    ImGui::PopItemWidth();
  }

  checkIpChange();

  ImGui::SameLine();
  ImGui::Checkbox("Ave##Show Ave", &ui::flag::showCpuAve);

  ImGui::SameLine();
  ImGui::Checkbox("Cores##Show Cores", &ui::flag::showCpuCores);

  ImGui::SameLine();
  ImGui::Checkbox("CPU##Show CPU", &ui::flag::showCpu);

  ImGui::SameLine();
  ImGui::Checkbox("MEM##Show MEM", &ui::flag::showMem);

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
                ->msg(std::string(name.c_str()))  // NOLINT
                ->rsp([](const std::string& msg) {
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
                ->msg(std::string(name.c_str()))  // NOLINT
                ->rsp([](const std::string& msg) {
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
                ->msg(std::string(pid.c_str()))  // NOLINT
                ->rsp([](const std::string& msg) {
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
                ->msg(std::string(pid.c_str()))  // NOLINT
                ->rsp([](const std::string& msg) {
                  LOGI("del pid rsp: %s", msg.c_str());
                })
                ->call();
          }
        }
      }
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    s_msg.clear();
    ui::flag::showTest = false;
    ui::flag::showLoadData = false;
  }

  ImGui::SameLine();
  if (ImGui::Button("Test")) {
    ui::flag::showTest = true;
    s_msg.createTestData();
  }

  ImGui::SameLine();
  if (ImGui::Button("Settings##settings_btn")) {
    ui::flag::showSettings = true;
  }
  static std::string error_msg;
  error_msg.resize(128);
  if (ui::flag::showSettings && ImGui::Begin("Settings##settings_window", &ui::flag::showSettings, ImGuiWindowFlags_NoCollapse)) {
    ImGui::Text("Load/Save settings:");

    {
      static std::string load_file_path;
      load_file_path.resize(128);
      ImGui::InputText("##load_file_path", (char*)load_file_path.data(), load_file_path.size());
      ImGui::SameLine();
      if (ImGui::Button("Load")) {
        bool ok;
        size_t size;
        auto file = file_utils::read_file(load_file_path.c_str(), &size, &ok);  // NOLINT(*-redundant-string-cstr)
        if (ok) {
          try {
            s_msg = nlohmann::json::parse((char*)file.get(), (char*)file.get() + size).get<MsgData>();
            ui::flag::showLoadData = true;
            error_msg = "load success!";
          } catch (std::exception& e) {
            LOGE("error: %s", e.what());
            error_msg = e.what();
          }
        } else {
          LOGE("error: open failed!");
          error_msg = "error: open failed!";
        }
      }
    }
    {
      static std::string save_file_path;
      save_file_path.resize(128);
      ImGui::InputText("##save_file_path", (char*)save_file_path.data(), save_file_path.size());
      ImGui::SameLine();
      if (ImGui::Button("Save")) {
        try {
          auto json = nlohmann::json(s_msg).dump(2);
          auto ok = file_utils::write_to_file(json.data(), json.size(), save_file_path.c_str());  // NOLINT(*-redundant-string-cstr)
          if (!ok) {
            error_msg = "save failed!";
          } else {
            error_msg = "save success!";
          }
        } catch (std::exception& e) {
          LOGE("error: %s", e.what());
          error_msg = e.what();
        }
      }
    }
    ImGui::Text("%s", error_msg.c_str());

    ImGui::End();
  } else {
    ui::flag::showSettings = false;
  }

  /*
  ImGui::SameLine();
  if (ImGui::Button("GetPids")) {
    if (s_rpc) {
      s_rpc->cmd("get_added_pids")
          ->rsp([](msg::ProcessMsg msg) {
            LOGI("get_added_pids rsp:");
            for (const auto& item : msg->infos) {
              LOGI("pid: %" PRIu64 ", name: %s", item.id, item.name.c_str());
            }
          })
          ->call();
    }
  }
  */

  ImGui::SameLine();
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

  // ave
  if (ui::flag::showCpuAve && ImPlot::BeginPlot("CPU Ave Usages (%/sec)")) {
    const int axisXMin = 10;
    ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

    ImPlot::SetupAxes("Time(sec)", "CPU(%)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_Lock);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);
    if (!s_msg_cpus.empty()) {
      ImPlot::PlotLineG(
          s_msg_cpus.front().ave.name.c_str(),
          (ImPlotGetter)[](int idx, void* user_data) {
            auto& info = s_msg_cpus[idx];
            return ImPlotPoint{calcTimestampsFromStart(info.timestamps), info.ave.usage};
          },
          nullptr, (int)s_msg_cpus.size());

      // shade
      ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
      ImPlot::PlotShadedG(
          s_msg_cpus.front().ave.name.c_str(),
          (ImPlotGetter)[](int idx, void* user_data) {
            auto& info = s_msg_cpus[idx];
            return ImPlotPoint{calcTimestampsFromStart(info.timestamps), 0};
          },
          nullptr,
          (ImPlotGetter)[](int idx, void* user_data) {
            auto& info = s_msg_cpus[idx];
            return ImPlotPoint{calcTimestampsFromStart(info.timestamps), info.ave.usage};
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

    ImPlot::SetupAxes("Time(sec)", "CPU(%)", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_Lock);
    if (!s_msg_cpus.empty()) {
      const uint32_t cpuCores = s_msg_cpus.front().cores.size();
      ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_None);
      for (uint32_t i = 0; i < cpuCores; ++i) {
        static uint32_t indexNow;
        indexNow = i;
        ImPlot::PlotLineG(
            s_msg_cpus.front().cores[indexNow].name.c_str(),
            (ImPlotGetter)[](int idx, void* user_data) {
              auto& info = s_msg_cpus[idx].cores[indexNow];
              return ImPlotPoint{calcTimestampsFromStart(info.timestamps), info.usage};
            },
            nullptr, (int)s_msg_cpus.size());
      }
    }
    ImPlot::EndPlot();
  }

  // pids
  {
    for (const auto& msgPid : s_msg_pids) {
      auto& processKey = msgPid.first;
      auto& processValue = msgPid.second;
      auto& threadInfoTable = processValue.thread_infos;

      // plot thread info
      if (ui::flag::showCpu) {
        auto plotName = "pid: " + std::to_string(processKey) + " name: " + processValue.name +
                        " threads: " + std::to_string(s_pid_current_thread_num[processKey]);
        if (!ImPlot::BeginPlot(plotName.c_str())) {
          break;
        }
        const int axisXMin = 10;
        ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

        int axisFlags = ImPlotAxisFlags_NoLabel;
        axisFlags |= ImPlotAxisFlags_AutoFit;

        ImPlot::SetupAxes("Time(sec)", "CPU(%)", axisFlags, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_None);
        if (!threadInfoTable.empty()) {
          for (auto& item : threadInfoTable) {
            const static ThreadInfosType* threadInfos;
            threadInfos = &(item.cpu_infos);

            auto labelName = std::string("tid: ") + std::to_string(threadInfos->front().id) + " name: " + threadInfos->front().name;
            ImPlot::PlotLineG(
                labelName.c_str(),
                (ImPlotGetter)[](int idx, void* user_data) {
                  auto& info = (*threadInfos)[idx];
                  return ImPlotPoint{calcTimestampsFromStart(info.timestamps), info.usage};
                },
                nullptr, (int)threadInfos->size());
          }
        }
        ImPlot::EndPlot();
      }

      // plot mem info
      if (ui::flag::showMem) {
        const static decltype(msgPid.second.mem_infos)* memInfos;
        memInfos = &(msgPid.second.mem_infos);

        auto plotName = "pid: " + std::to_string(processKey) + " name: " + processValue.name + " Memory/MB";
        if (!ImPlot::BeginPlot(plotName.c_str())) {
          break;
        }

        const int axisXMin = 10;
        ImPlot::SetupAxesLimits(0, axisXMin, 0, 100);

        int axisFlags = ImPlotAxisFlags_NoLabel;
        axisFlags |= ImPlotAxisFlags_AutoFit;

        ImPlot::SetupAxes("Time(sec)", "Memory(MB)", axisFlags, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_None);

        char label_tmp[128];
// #define CPUMONITOR_ENABLE_VMHWM
#ifdef CPUMONITOR_ENABLE_VMHWM
        snprintf(label_tmp, sizeof(label_tmp), "VmHWM: %.2fMB(%zuKB)", (float)memInfos->back().hwm / 1024, (size_t)memInfos->back().hwm);
        ImPlot::PlotLineG(
            label_tmp,
            (ImPlotGetter)[](int idx, void* user_data) {
              auto& info = (*memInfos)[idx];
              return ImPlotPoint{calcTimestampsFromStart(info.timestamps), (float)info.hwm / 1024};
            },
            nullptr, (int)memInfos->size());
#endif
        snprintf(label_tmp, sizeof(label_tmp), "VmRSS: %.2fMB(%zuKB) MAX:%.2fMB", (float)memInfos->back().rss / 1024, (size_t)memInfos->back().rss,
                 (float)msgPid.second.max_rss / 1024);
        ImPlot::PlotLineG(
            label_tmp,
            (ImPlotGetter)[](int idx, void* user_data) {
              auto& info = (*memInfos)[idx];
              return ImPlotPoint{calcTimestampsFromStart(info.timestamps), (float)info.rss / 1024};
            },
            nullptr, (int)memInfos->size());
        ImPlot::EndPlot();
      }
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
  initRpc();
  initClient();
}

Home::~Home() {
  LOGI("~Home");
  s_rpc_client->close();
  ImPlot::DestroyContext();
}
