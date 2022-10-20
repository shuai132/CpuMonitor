#include <thread>

#include "TimeTracker.hpp"

int main() {
  std::thread([] {
    pthread_setname_np(pthread_self(), "cpu_test_1");
    for (;;) {
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    };
  }).detach();

  std::thread([] {
    pthread_setname_np(pthread_self(), "cpu_test_2");
    for (;;) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    };
  }).detach();

  std::thread([] {
    pthread_setname_np(pthread_self(), "cpu_test_3");
    for (;;) {
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
    };
  }).detach();

  // 模拟动态得新增和删除线程
  for (;;) {
    const int switchThreadSec = 5;
    std::thread([] {
      pthread_setname_np(pthread_self(), "cpu_test_x");

      TimeTracker tracker;
      while (tracker.sec() < switchThreadSec) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    }).join();
    std::this_thread::sleep_for(std::chrono::seconds(switchThreadSec));
  }

  // getchar();

  return 0;
}
