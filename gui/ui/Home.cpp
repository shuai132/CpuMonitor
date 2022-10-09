#include "Home.h"

#include "imgui.h"
#include "uidef.h"

using namespace ImGui;

void Home::onDraw() const {
  ImGui::Begin("MainWindow", nullptr, windowFlags_);  // NOLINT
  ImGui::SetWindowPos({0, 0});
  ImGui::SetWindowSize({ui::MainWindowWidth, ui::MainWindowHeight});
  PushItemWidth(ui::ITEM_WIDTH);

  Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GetIO().Framerate, GetIO().Framerate);

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
