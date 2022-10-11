#include "Home.h"

#include "App.h"
#include "imgui.h"
#include "log.h"
#include "rpc_client.hpp"
#include "uidef.h"

using namespace ImGui;

// rpc
static std::unique_ptr<asio_net::rpc_client> s_rpc_client;
static std::shared_ptr<RpcCore::Rpc> s_rpc;
static std::shared_ptr<RpcCore::Dispose> s_dispose;

void Home::onDraw() const {
  ImGui::Begin("MainWindow", nullptr, windowFlags_);  // NOLINT
  ImGui::SetWindowPos({0, 0});
  ImGui::SetWindowSize({ui::MainWindowWidth, ui::MainWindowHeight});
  PushItemWidth(ui::ITEM_WIDTH);

  Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GetIO().Framerate, GetIO().Framerate);

  if (Button("connect")) {
    s_rpc_client = std::make_unique<asio_net::rpc_client>(App::instance()->context());
    s_rpc_client->on_open = [](std::shared_ptr<RpcCore::Rpc> rpc) {
      LOGI("on_open");
      s_rpc = std::move(rpc);
    };
    LOGI("try open...");
    s_rpc_client->open("10.238.21.156", "8088");
  }

  PopItemWidth();
  ImGui::End();
}

void Home::initGUI() {
  windowFlags_ |= ImGuiWindowFlags_NoMove;
  windowFlags_ |= ImGuiWindowFlags_NoResize;
  windowFlags_ |= ImGuiWindowFlags_NoCollapse;
  windowFlags_ |= ImGuiWindowFlags_NoTitleBar;
}

Home::Home() {
  initGUI();
}
