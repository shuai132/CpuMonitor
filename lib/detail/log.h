// 可配置项（默认未定义）
// cpu_monitor_LOG_NDEBUG               关闭cpu_monitor_LOGD的输出
// cpu_monitor_LOG_SHOW_VERBOSE         显示cpu_monitor_LOGV的输出
// cpu_monitor_LOG_DISABLE_COLOR        禁用颜色显示
// cpu_monitor_LOG_LINE_END_CRLF        默认是\n结尾 添加此宏将以\r\n结尾
// cpu_monitor_LOG_FOR_MCU              更适用于MCU环境
// cpu_monitor_LOG_NOT_EXIT_ON_FATAL    FATAL默认退出程序 添加此宏将不退出
//
// c++11环境默认打开以下内容
// cpu_monitor_LOG_ENABLE_THREAD_SAFE   线程安全
// cpu_monitor_LOG_ENABLE_THREAD_ID     显示线程ID
// cpu_monitor_LOG_ENABLE_DATE_TIME     显示日期
// 分别可通过下列禁用
// cpu_monitor_LOG_DISABLE_THREAD_SAFE
// cpu_monitor_LOG_DISABLE_THREAD_ID
// cpu_monitor_LOG_DISABLE_DATE_TIME
//
// 其他配置项
// cpu_monitor_LOG_PRINTF_IMPL          定义输出实现（默认使用printf）
// 并添加形如int cpu_monitor_LOG_PRINTF_IMPL(const char *fmt, ...)的实现
//
// 在库中使用时
// 1. 修改此文件中的`cpu_monitor_LOG`以包含库名前缀（全部替换即可）
// 2. 取消这行注释: #define cpu_monitor_LOG_IN_LIB
// 库中可配置项
// cpu_monitor_LOG_SHOW_DEBUG           开启cpu_monitor_LOGD的输出
//
// 非库中使用时
// cpu_monitor_LOGD的输出在debug时打开 release时关闭（依据NDEBUG宏）

#pragma once

// clang-format off

// 自定义配置
//#include "log_config.h"

// 在库中使用时需取消注释
#define cpu_monitor_LOG_IN_LIB

#ifdef __cplusplus
#include <cstring>
#include <cstdlib>
#if __cplusplus >= 201103L

#if !defined(cpu_monitor_LOG_DISABLE_THREAD_SAFE) && !defined(cpu_monitor_LOG_ENABLE_THREAD_SAFE)
#define cpu_monitor_LOG_ENABLE_THREAD_SAFE
#endif

#if !defined(cpu_monitor_LOG_DISABLE_THREAD_ID) && !defined(cpu_monitor_LOG_ENABLE_THREAD_ID)
#define cpu_monitor_LOG_ENABLE_THREAD_ID
#endif

#if !defined(cpu_monitor_LOG_DISABLE_DATE_TIME) && !defined(cpu_monitor_LOG_ENABLE_DATE_TIME)
#define cpu_monitor_LOG_ENABLE_DATE_TIME
#endif

#endif
#else
#include <string.h>
#include <stdlib.h>
#endif

#ifdef  cpu_monitor_LOG_LINE_END_CRLF
#define cpu_monitor_LOG_LINE_END            "\r\n"
#else
#define cpu_monitor_LOG_LINE_END            "\n"
#endif

#ifdef cpu_monitor_LOG_NOT_EXIT_ON_FATAL
#define cpu_monitor_LOG_EXIT_PROGRAM()
#else
#ifdef cpu_monitor_LOG_FOR_MCU
#define cpu_monitor_LOG_EXIT_PROGRAM()      do{ for(;;); } while(0)
#else
#define cpu_monitor_LOG_EXIT_PROGRAM()      exit(EXIT_FAILURE)
#endif
#endif

#define cpu_monitor_LOG_BASE_FILENAME       (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define cpu_monitor_LOG_WITH_COLOR

#if defined(_WIN32) || defined(__ANDROID__) || defined(cpu_monitor_LOG_FOR_MCU)
#undef cpu_monitor_LOG_WITH_COLOR
#endif

#ifdef cpu_monitor_LOG_DISABLE_COLOR
#undef cpu_monitor_LOG_WITH_COLOR
#endif

#ifdef cpu_monitor_LOG_WITH_COLOR
#define cpu_monitor_LOG_COLOR_RED           "\033[31m"
#define cpu_monitor_LOG_COLOR_GREEN         "\033[32m"
#define cpu_monitor_LOG_COLOR_YELLOW        "\033[33m"
#define cpu_monitor_LOG_COLOR_BLUE          "\033[34m"
#define cpu_monitor_LOG_COLOR_CARMINE       "\033[35m"
#define cpu_monitor_LOG_COLOR_CYAN          "\033[36m"
#define cpu_monitor_LOG_COLOR_DEFAULT
#define cpu_monitor_LOG_COLOR_END           "\033[m"
#else
#define cpu_monitor_LOG_COLOR_RED
#define cpu_monitor_LOG_COLOR_GREEN
#define cpu_monitor_LOG_COLOR_YELLOW
#define cpu_monitor_LOG_COLOR_BLUE
#define cpu_monitor_LOG_COLOR_CARMINE
#define cpu_monitor_LOG_COLOR_CYAN
#define cpu_monitor_LOG_COLOR_DEFAULT
#define cpu_monitor_LOG_COLOR_END
#endif

#define cpu_monitor_LOG_END                 cpu_monitor_LOG_COLOR_END cpu_monitor_LOG_LINE_END

#if __ANDROID__
#include <android/log.h>
#define cpu_monitor_LOG_PRINTF(...)         __android_log_print(ANDROID_L##OG_DEBUG, "cpu_monitor_LOG", __VA_ARGS__)
#else
#define cpu_monitor_LOG_PRINTF(...)         printf(__VA_ARGS__)
#endif

#ifndef cpu_monitor_LOG_PRINTF_IMPL
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#ifdef cpu_monitor_LOG_ENABLE_THREAD_SAFE
#include <mutex>
struct cpu_monitor_LOG_Mutex {
static std::mutex& mutex() {
static std::mutex mutex;
return mutex;
}
};
#define cpu_monitor_LOG_PRINTF_IMPL(...)    \
std::lock_guard<std::mutex> lock(cpu_monitor_LOG_Mutex::mutex()); \
cpu_monitor_LOG_PRINTF(__VA_ARGS__)
#else
#define cpu_monitor_LOG_PRINTF_IMPL(...)    cpu_monitor_LOG_PRINTF(__VA_ARGS__)
#endif

#else
extern int cpu_monitor_LOG_PRINTF_IMPL(const char *fmt, ...);
#endif

#ifdef cpu_monitor_LOG_ENABLE_THREAD_ID
#include <thread>
#include <sstream>
#include <string>
namespace cpu_monitor_LOG {
inline std::string get_thread_id() {
std::stringstream ss;
ss << std::this_thread::get_id();
return ss.str();
}
}
#define cpu_monitor_LOG_THREAD_LABEL "%s "
#define cpu_monitor_LOG_THREAD_VALUE ,cpu_monitor_LOG::get_thread_id().c_str()
#else
#define cpu_monitor_LOG_THREAD_LABEL
#define cpu_monitor_LOG_THREAD_VALUE
#endif

#ifdef cpu_monitor_LOG_ENABLE_DATE_TIME
#include <chrono>
#include <sstream>
#include <iomanip>
namespace cpu_monitor_LOG {
inline std::string get_time() {
auto now = std::chrono::system_clock::now();
std::time_t time = std::chrono::system_clock::to_time_t(now);
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
std::stringstream ss;
ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
return ss.str();
}
}
#define cpu_monitor_LOG_TIME_LABEL "%s "
#define cpu_monitor_LOG_TIME_VALUE ,cpu_monitor_LOG::get_time().c_str()
#else
#define cpu_monitor_LOG_TIME_LABEL
#define cpu_monitor_LOG_TIME_VALUE
#endif

#define cpu_monitor_LOG(fmt, ...)           do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_GREEN   cpu_monitor_LOG_TIME_LABEL cpu_monitor_LOG_THREAD_LABEL "[*]: %s:%d "       fmt cpu_monitor_LOG_END cpu_monitor_LOG_TIME_VALUE cpu_monitor_LOG_THREAD_VALUE, cpu_monitor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define cpu_monitor_LOGT(tag, fmt, ...)     do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_BLUE    cpu_monitor_LOG_TIME_LABEL cpu_monitor_LOG_THREAD_LABEL "[" tag "]: %s:%d " fmt cpu_monitor_LOG_END cpu_monitor_LOG_TIME_VALUE cpu_monitor_LOG_THREAD_VALUE, cpu_monitor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define cpu_monitor_LOGI(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_YELLOW  cpu_monitor_LOG_TIME_LABEL cpu_monitor_LOG_THREAD_LABEL "[I]: %s:%d "       fmt cpu_monitor_LOG_END cpu_monitor_LOG_TIME_VALUE cpu_monitor_LOG_THREAD_VALUE, cpu_monitor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define cpu_monitor_LOGW(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_CARMINE cpu_monitor_LOG_TIME_LABEL cpu_monitor_LOG_THREAD_LABEL "[W]: %s:%d [%s] "  fmt cpu_monitor_LOG_END cpu_monitor_LOG_TIME_VALUE cpu_monitor_LOG_THREAD_VALUE, cpu_monitor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define cpu_monitor_LOGE(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_RED     cpu_monitor_LOG_TIME_LABEL cpu_monitor_LOG_THREAD_LABEL "[E]: %s:%d [%s] "  fmt cpu_monitor_LOG_END cpu_monitor_LOG_TIME_VALUE cpu_monitor_LOG_THREAD_VALUE, cpu_monitor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define cpu_monitor_LOGF(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_CYAN    cpu_monitor_LOG_TIME_LABEL cpu_monitor_LOG_THREAD_LABEL "[!]: %s:%d [%s] "  fmt cpu_monitor_LOG_END cpu_monitor_LOG_TIME_VALUE cpu_monitor_LOG_THREAD_VALUE, cpu_monitor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); cpu_monitor_LOG_EXIT_PROGRAM(); } while(0) // NOLINT(bugprone-lambda-function-name)

#if defined(cpu_monitor_LOG_IN_LIB) && !defined(cpu_monitor_LOG_SHOW_DEBUG) && !defined(cpu_monitor_LOG_NDEBUG)
#define cpu_monitor_LOG_NDEBUG
#endif

#if defined(NDEBUG) || defined(cpu_monitor_LOG_NDEBUG)
#define cpu_monitor_LOGD(fmt, ...)          ((void)0)
#else
#define cpu_monitor_LOGD(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_DEFAULT cpu_monitor_LOG_TIME_LABEL cpu_monitor_LOG_THREAD_LABEL "[D]: %s:%d "       fmt cpu_monitor_LOG_END cpu_monitor_LOG_TIME_VALUE cpu_monitor_LOG_THREAD_VALUE, cpu_monitor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#endif

#if defined(cpu_monitor_LOG_SHOW_VERBOSE)
#define cpu_monitor_LOGV(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_DEFAULT "[V]: %s: "         fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, ##__VA_ARGS__); } while(0)
#else
#define cpu_monitor_LOGV(fmt, ...)          ((void)0)
#endif
