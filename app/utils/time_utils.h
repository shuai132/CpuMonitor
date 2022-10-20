#pragma once

#include <chrono>

namespace utils {

uint64_t getTimestamps() {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
  return ms.count();
}

}  // namespace utils
