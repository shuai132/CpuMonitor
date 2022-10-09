#pragma once

#include <functional>
#include <utility>

namespace detail {

class DeferHelper {
 public:
  explicit DeferHelper(std::function<void()> func) : func_(std::move(func)) {}
  ~DeferHelper() {
    func_();
  }

  DeferHelper(const DeferHelper&) = delete;
  void operator=(const DeferHelper&) = delete;
  DeferHelper(DeferHelper&&) = default;

 private:
  std::function<void()> func_;
};

struct DeferOp {
  template <typename Func>
  DeferHelper operator<<(Func&& func) {
    return DeferHelper(std::forward<Func>(func));
  }
};

#define __DEFER_CAT_(a, b) a##b
#define __DEFER_CAT(a, b) __DEFER_CAT_(a, b)
#define defer auto __DEFER_CAT(__defer__, __COUNTER__) = detail::DeferOp() << [&]() -> void

}  // namespace detail
