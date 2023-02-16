/**
 * 统一控制调试信息
 * 为了保证输出顺序 都使用stdout而不是stderr
 *
 * 可配置项（默认都是未定义）
 * cpu_monitor_LOG_NDEBUG               关闭cpu_monitor_LOGD的输出
 * cpu_monitor_LOG_SHOW_VERBOSE         显示cpu_monitor_LOGV的输出
 * cpu_monitor_LOG_DISABLE_COLOR        禁用颜色显示
 * cpu_monitor_LOG_LINE_END_CRLF        默认是\n结尾 添加此宏将以\r\n结尾
 * cpu_monitor_LOG_FOR_MCU              MCU项目可配置此宏 更适用于MCU环境
 * cpu_monitor_LOG_NOT_EXIT_ON_FATAL    FATAL默认退出程序 添加此宏将不退出
 *
 * 其他配置项
 * cpu_monitor_LOG_PRINTF_IMPL          定义输出实现（默认使用printf）
 * 并添加形如int cpu_monitor_LOG_PRINTF_IMPL(const char *fmt, ...)的实现
 *
 * 在库中使用时
 * 1. 修改此文件中的`cpu_monitor_LOG`以包含库名前缀（全部替换即可）
 * 2. 取消这行注释: #define cpu_monitor_LOG_IN_LIB
 * 库中可配置项
 * cpu_monitor_LOG_SHOW_DEBUG           开启cpu_monitor_LOGD的输出
 *
 * 非库中使用时
 * cpu_monitor_LOGD的输出在debug时打开 release时关闭（依据NDEBUG宏）
 */

#pragma once

// clang-format off

// 在库中使用时需取消注释
#define cpu_monitor_LOG_IN_LIB

#ifdef __cplusplus
#include <cstring>
#include <cstdlib>
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
#define cpu_monitor_LOG_PRINTF_IMPL(...)    cpu_monitor_LOG_PRINTF(__VA_ARGS__) // NOLINT(bugprone-lambda-function-name)
#else
extern int cpu_monitor_LOG_PRINTF_IMPL(const char *fmt, ...);
#endif

#define cpu_monitor_LOG(fmt, ...)           do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_GREEN   "[*]: %s:%d "       fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define cpu_monitor_LOGT(tag, fmt, ...)     do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_BLUE    "[" tag "]: %s:%d " fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define cpu_monitor_LOGI(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_YELLOW  "[I]: %s:%d "       fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define cpu_monitor_LOGW(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_CARMINE "[W]: %s:%d [%s] "  fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define cpu_monitor_LOGE(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_RED     "[E]: %s:%d [%s] "  fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define cpu_monitor_LOGF(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_CYAN    "[!]: %s:%d [%s] "  fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); cpu_monitor_LOG_EXIT_PROGRAM(); } while(0) // NOLINT(bugprone-lambda-function-name)

#if defined(cpu_monitor_LOG_IN_LIB) && !defined(cpu_monitor_LOG_SHOW_DEBUG) && !defined(cpu_monitor_LOG_NDEBUG)
#define cpu_monitor_LOG_NDEBUG
#endif

#if defined(NDEBUG) || defined(cpu_monitor_LOG_NDEBUG)
#define cpu_monitor_LOGD(fmt, ...)          ((void)0)
#else
#define cpu_monitor_LOGD(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_DEFAULT "[D]: %s:%d "       fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#endif

#if defined(cpu_monitor_LOG_SHOW_VERBOSE)
#define cpu_monitor_LOGV(fmt, ...)          do{ cpu_monitor_LOG_PRINTF_IMPL(cpu_monitor_LOG_COLOR_DEFAULT "[V]: %s: "         fmt cpu_monitor_LOG_END, cpu_monitor_LOG_BASE_FILENAME, ##__VA_ARGS__); } while(0)
#else
#define cpu_monitor_LOGV(fmt, ...)          ((void)0)
#endif
