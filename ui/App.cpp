#include "App.h"

#include "log.h"
#include "ui/Home.h"

static std::unique_ptr<Home> home;

App* App::instance() {
  static App instance;
  return &instance;
}

App::App() {
  context_.post([] {
    home = std::make_unique<Home>();
  });
}

App::~App() {
  LOGI("~App");
  home = nullptr;
}

void App::poll() {
  context_.poll();
  home->onDraw();
}

void App::dispatch(const std::function<void()>& task) {
  context_.dispatch(task);
}

void App::post(std::function<void()> task) {
  context_.post(std::move(task));
}
