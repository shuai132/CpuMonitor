#pragma once

namespace cpu_monitor {
namespace detail {

class noncopyable {
 protected:
  noncopyable() = default;
  ~noncopyable() = default;

 public:
  noncopyable(const noncopyable&) = delete;
  const noncopyable& operator=(const noncopyable&) = delete;
};

}  // namespace detail
}  // namespace cpu_monitor
