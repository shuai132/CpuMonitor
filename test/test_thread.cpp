#include "thread"

int main() {
  std::thread([] {
    for (;;) {
      pthread_setname_np(pthread_self(), "cpu_test_1");
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
    std::thread([] {
      pthread_setname_np(pthread_self(), "cpu_test_x");
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }).join();
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }

  getchar();

  return 0;
}
