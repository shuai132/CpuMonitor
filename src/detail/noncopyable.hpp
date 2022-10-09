#pragma once

namespace cpu_monitor {

class noncopyable {
 protected:
  noncopyable() = default;
  ~noncopyable() = default;

 public:
  noncopyable(const noncopyable&) = delete;
  const noncopyable& operator=(const noncopyable&) = delete;
};

}  // namespace cpu_monitor
