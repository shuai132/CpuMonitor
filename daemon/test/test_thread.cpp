#include <thread>

#include "TimeTracker.hpp"

static inline void set_thread_name(const char* name) {
#ifdef __linux__
  pthread_setname_np(pthread_self(), name);
#elif defined(__APPLE__)
  pthread_setname_np(name);
#endif
}

int main() {
  std::thread([] {
    set_thread_name("cpu_test_1");
    for (;;) {
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  }).detach();

  std::thread([] {
    set_thread_name("cpu_test_2");
    for (;;) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }).detach();

  std::thread([] {
    set_thread_name("cpu_test_3");
    for (;;) {
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
  }).detach();

  // 模拟动态得新增和删除线程
  for (;;) {
    const int switchThreadSec = 5;
    std::thread([] {
      set_thread_name("cpu_test_x");

      TimeTracker tracker;
      while (tracker.sec() < switchThreadSec) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
    }).join();
    std::this_thread::sleep_for(std::chrono::seconds(switchThreadSec));
  }

  return 0;
}
