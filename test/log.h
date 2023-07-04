// 可配置项（默认未定义）
// LOG_NDEBUG               关闭LOGD的输出
// LOG_SHOW_VERBOSE         显示LOGV的输出
// LOG_DISABLE_COLOR        禁用颜色显示
// LOG_LINE_END_CRLF        默认是\n结尾 添加此宏将以\r\n结尾
// LOG_FOR_MCU              更适用于MCU环境
// LOG_NOT_EXIT_ON_FATAL    FATAL默认退出程序 添加此宏将不退出
//
// c++11环境默认打开以下内容
// LOG_ENABLE_THREAD_SAFE   线程安全
// LOG_ENABLE_THREAD_ID     显示线程ID
// LOG_ENABLE_DATE_TIME     显示日期
// 分别可通过下列禁用
// LOG_DISABLE_THREAD_SAFE
// LOG_DISABLE_THREAD_ID
// LOG_DISABLE_DATE_TIME
//
// 其他配置项
// LOG_PRINTF_IMPL          定义输出实现（默认使用printf）
// 并添加形如int LOG_PRINTF_IMPL(const char *fmt, ...)的实现
//
// 在库中使用时
// 1. 修改此文件中的`LOG`以包含库名前缀（全部替换即可）
// 2. 取消这行注释: #define LOG_IN_LIB
// 库中可配置项
// LOG_SHOW_DEBUG           开启LOGD的输出
//
// 非库中使用时
// LOGD的输出在debug时打开 release时关闭（依据NDEBUG宏）

#pragma once

// clang-format off

// 自定义配置
//#include "log_config.h"

// 在库中使用时需取消注释
//#define LOG_IN_LIB

#ifdef __cplusplus
#include <cstring>
#include <cstdlib>
#if __cplusplus >= 201103L

#if !defined(LOG_DISABLE_THREAD_SAFE) && !defined(LOG_ENABLE_THREAD_SAFE)
#define LOG_ENABLE_THREAD_SAFE
#endif

#if !defined(LOG_DISABLE_THREAD_ID) && !defined(LOG_ENABLE_THREAD_ID)
#define LOG_ENABLE_THREAD_ID
#endif

#if !defined(LOG_DISABLE_DATE_TIME) && !defined(LOG_ENABLE_DATE_TIME)
#define LOG_ENABLE_DATE_TIME
#endif

#endif
#else
#include <string.h>
#include <stdlib.h>
#endif

#ifdef  LOG_LINE_END_CRLF
#define LOG_LINE_END            "\r\n"
#else
#define LOG_LINE_END            "\n"
#endif

#ifdef LOG_NOT_EXIT_ON_FATAL
#define LOG_EXIT_PROGRAM()
#else
#ifdef LOG_FOR_MCU
#define LOG_EXIT_PROGRAM()      do{ for(;;); } while(0)
#else
#define LOG_EXIT_PROGRAM()      exit(EXIT_FAILURE)
#endif
#endif

#define LOG_BASE_FILENAME       (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define LOG_WITH_COLOR

#if defined(_WIN32) || (defined(__ANDROID__) && !defined(ANDROID_STANDALONE)) || defined(LOG_FOR_MCU)
#undef LOG_WITH_COLOR
#endif

#ifdef LOG_DISABLE_COLOR
#undef LOG_WITH_COLOR
#endif

#ifdef LOG_WITH_COLOR
#define LOG_COLOR_RED           "\033[31m"
#define LOG_COLOR_GREEN         "\033[32m"
#define LOG_COLOR_YELLOW        "\033[33m"
#define LOG_COLOR_BLUE          "\033[34m"
#define LOG_COLOR_CARMINE       "\033[35m"
#define LOG_COLOR_CYAN          "\033[36m"
#define LOG_COLOR_DEFAULT
#define LOG_COLOR_END           "\033[m"
#else
#define LOG_COLOR_RED
#define LOG_COLOR_GREEN
#define LOG_COLOR_YELLOW
#define LOG_COLOR_BLUE
#define LOG_COLOR_CARMINE
#define LOG_COLOR_CYAN
#define LOG_COLOR_DEFAULT
#define LOG_COLOR_END
#endif

#define LOG_END                 LOG_COLOR_END LOG_LINE_END

#if defined(__ANDROID__) && !defined(ANDROID_STANDALONE)
#include <android/log.h>
#define LOG_PRINTF(...)         __android_log_print(ANDROID_L##OG_DEBUG, "LOG", __VA_ARGS__)
#else
#define LOG_PRINTF(...)         printf(__VA_ARGS__)
#endif

#ifndef LOG_PRINTF_IMPL
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#ifdef LOG_ENABLE_THREAD_SAFE
#include <mutex>
struct LOG_Mutex {
static std::mutex& mutex() {
static std::mutex mutex;
return mutex;
}
};
#define LOG_PRINTF_IMPL(...)    \
std::lock_guard<std::mutex> lock(LOG_Mutex::mutex()); \
LOG_PRINTF(__VA_ARGS__)
#else
#define LOG_PRINTF_IMPL(...)    LOG_PRINTF(__VA_ARGS__)
#endif

#else
extern int LOG_PRINTF_IMPL(const char *fmt, ...);
#endif

#ifdef LOG_ENABLE_THREAD_ID
#include <thread>
#include <sstream>
#include <string>
namespace LOG {
inline std::string get_thread_id() {
std::stringstream ss;
ss << std::this_thread::get_id();
return ss.str();
}
}
#define LOG_THREAD_LABEL "%s "
#define LOG_THREAD_VALUE ,LOG::get_thread_id().c_str()
#else
#define LOG_THREAD_LABEL
#define LOG_THREAD_VALUE
#endif

#ifdef LOG_ENABLE_DATE_TIME
#include <chrono>
#include <sstream>
#include <iomanip>
namespace LOG {
inline std::string get_time() {
auto now = std::chrono::system_clock::now();
std::time_t time = std::chrono::system_clock::to_time_t(now);
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
std::stringstream ss;
ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
return ss.str();
}
}
#define LOG_TIME_LABEL "%s "
#define LOG_TIME_VALUE ,LOG::get_time().c_str()
#else
#define LOG_TIME_LABEL
#define LOG_TIME_VALUE
#endif

#define LOG(fmt, ...)           do{ LOG_PRINTF_IMPL(LOG_COLOR_GREEN   LOG_TIME_LABEL LOG_THREAD_LABEL "[*]: %s:%d "       fmt LOG_END LOG_TIME_VALUE LOG_THREAD_VALUE, LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define LOGT(tag, fmt, ...)     do{ LOG_PRINTF_IMPL(LOG_COLOR_BLUE    LOG_TIME_LABEL LOG_THREAD_LABEL "[" tag "]: %s:%d " fmt LOG_END LOG_TIME_VALUE LOG_THREAD_VALUE, LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define LOGI(fmt, ...)          do{ LOG_PRINTF_IMPL(LOG_COLOR_YELLOW  LOG_TIME_LABEL LOG_THREAD_LABEL "[I]: %s:%d "       fmt LOG_END LOG_TIME_VALUE LOG_THREAD_VALUE, LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define LOGW(fmt, ...)          do{ LOG_PRINTF_IMPL(LOG_COLOR_CARMINE LOG_TIME_LABEL LOG_THREAD_LABEL "[W]: %s:%d [%s] "  fmt LOG_END LOG_TIME_VALUE LOG_THREAD_VALUE, LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define LOGE(fmt, ...)          do{ LOG_PRINTF_IMPL(LOG_COLOR_RED     LOG_TIME_LABEL LOG_THREAD_LABEL "[E]: %s:%d [%s] "  fmt LOG_END LOG_TIME_VALUE LOG_THREAD_VALUE, LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define LOGF(fmt, ...)          do{ LOG_PRINTF_IMPL(LOG_COLOR_CYAN    LOG_TIME_LABEL LOG_THREAD_LABEL "[!]: %s:%d [%s] "  fmt LOG_END LOG_TIME_VALUE LOG_THREAD_VALUE, LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); LOG_EXIT_PROGRAM(); } while(0) // NOLINT(bugprone-lambda-function-name)

#if defined(LOG_IN_LIB) && !defined(LOG_SHOW_DEBUG) && !defined(LOG_NDEBUG)
#define LOG_NDEBUG
#endif

#if defined(NDEBUG) || defined(LOG_NDEBUG)
#define LOGD(fmt, ...)          ((void)0)
#else
#define LOGD(fmt, ...)          do{ LOG_PRINTF_IMPL(LOG_COLOR_DEFAULT LOG_TIME_LABEL LOG_THREAD_LABEL "[D]: %s:%d "       fmt LOG_END LOG_TIME_VALUE LOG_THREAD_VALUE, LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#endif

#if defined(LOG_SHOW_VERBOSE)
#define LOGV(fmt, ...)          do{ LOG_PRINTF_IMPL(LOG_COLOR_DEFAULT LOG_TIME_LABEL LOG_THREAD_LABEL "[V]: %s:%d "       fmt LOG_END LOG_TIME_VALUE LOG_THREAD_VALUE, LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#else
#define LOGV(fmt, ...)          ((void)0)
#endif
