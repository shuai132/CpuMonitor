#pragma once

#include "App.h"

/**
 * 投递到主线程执行runnable
 * 不论当前是否在主线程 都将投递到事件队列末尾
 *
 * @param runnable
 */
inline void PostMainThread(App::Runnable runnable) {
  App::instance()->post(std::move(runnable));
}

/**
 * 在主线程执行runnable
 * 若当前已在主线程则直接执行
 *
 * @param runnable
 */
inline void RunOnMainThread(const App::Runnable& runnable) {
  App::instance()->dispatch(runnable);
}

/**
 * 在主线程执行runnable 并阻塞直到获取返回值或异常
 * 若当前已在主线程则直接执行
 *
 * @tparam T
 * @tparam R
 * @param runnable
 * @return
 */
template <typename T, typename R = typename std::result_of<T()>::type>
inline typename std::enable_if<!std::is_same<R, void>::value, R>::type RunOnMainThreadBlocking(const T& runnable) {
  std::promise<R> ret;
  RunOnMainThread([&] {
    try {
      ret.set_value(runnable());
    } catch (...) {
      ret.set_exception(std::current_exception());
    }
  });
  return ret.get_future().get();
}

template <typename T, typename R = typename std::result_of<T()>::type>
inline typename std::enable_if<std::is_same<R, void>::value, void>::type RunOnMainThreadBlocking(const T& runnable) {
  std::promise<R> ret;
  RunOnMainThread([&] {
    try {
      runnable();
      ret.set_value();
    } catch (...) {
      ret.set_exception(std::current_exception());
    }
  });
  ret.get_future().get();
}
