#include "App.h"

#include "imgui.h"
#include "ui/uidef.h"

using namespace ImGui;

App* App::instance() {
  static App instance;
  return &instance;
}

App::App() {
  initGUI();
}

void App::onDraw() {
  context_.poll();
  context_.restart();

  ImGui::Begin("MainWindow", nullptr, windowFlags_);  // NOLINT
  ImGui::SetWindowPos({0, 0});
  ImGui::SetWindowSize({MainWindowWidth, MainWindowHeight});
  PushItemWidth(UI_ITEM_WIDTH);

  Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GetIO().Framerate, GetIO().Framerate);

  PopItemWidth();
  ImGui::End();
}

void App::dispatch(const std::function<void()>& task) {
  context_.dispatch(task);
}

void App::post(std::function<void()> task) {
  context_.post(std::move(task));
}

void App::initGUI() {
  windowFlags_ |= ImGuiWindowFlags_NoMove;
  windowFlags_ |= ImGuiWindowFlags_NoResize;
  windowFlags_ |= ImGuiWindowFlags_NoCollapse;
  windowFlags_ |= ImGuiWindowFlags_NoTitleBar;
}

const char* App::MainWindowTitle = "CpuMonitor";
