// 可配置项（默认未定义）
// ASIO_NET_LOG_NDEBUG               关闭ASIO_NET_LOGD的输出
// ASIO_NET_LOG_SHOW_VERBOSE         显示ASIO_NET_LOGV的输出
// ASIO_NET_LOG_DISABLE_COLOR        禁用颜色显示
// ASIO_NET_LOG_LINE_END_CRLF        默认是\n结尾 添加此宏将以\r\n结尾
// ASIO_NET_LOG_FOR_MCU              更适用于MCU环境
// ASIO_NET_LOG_NOT_EXIT_ON_FATAL    FATAL默认退出程序 添加此宏将不退出
//
// c++11环境默认打开以下内容
// ASIO_NET_LOG_ENABLE_THREAD_SAFE   线程安全
// ASIO_NET_LOG_ENABLE_THREAD_ID     显示线程ID
// ASIO_NET_LOG_ENABLE_DATE_TIME     显示日期
// 分别可通过下列禁用
// ASIO_NET_LOG_DISABLE_THREAD_SAFE
// ASIO_NET_LOG_DISABLE_THREAD_ID
// ASIO_NET_LOG_DISABLE_DATE_TIME
//
// 其他配置项
// ASIO_NET_LOG_PRINTF_IMPL          定义输出实现（默认使用printf）
// 并添加形如int ASIO_NET_LOG_PRINTF_IMPL(const char *fmt, ...)的实现
//
// 在库中使用时
// 1. 修改此文件中的`ASIO_NET_LOG`以包含库名前缀（全部替换即可）
// 2. 取消这行注释: #define ASIO_NET_LOG_IN_LIB
// 库中可配置项
// ASIO_NET_LOG_SHOW_DEBUG           开启ASIO_NET_LOGD的输出
//
// 非库中使用时
// ASIO_NET_LOGD的输出在debug时打开 release时关闭（依据NDEBUG宏）

#pragma once

// clang-format off

// 自定义配置
//#include "log_config.h"

// 在库中使用时需取消注释
#define ASIO_NET_LOG_IN_LIB

#ifdef __cplusplus
#include <cstring>
#include <cstdlib>
#if __cplusplus >= 201103L

#if !defined(ASIO_NET_LOG_DISABLE_THREAD_SAFE) && !defined(ASIO_NET_LOG_ENABLE_THREAD_SAFE)
#define ASIO_NET_LOG_ENABLE_THREAD_SAFE
#endif

#if !defined(ASIO_NET_LOG_DISABLE_THREAD_ID) && !defined(ASIO_NET_LOG_ENABLE_THREAD_ID)
#define ASIO_NET_LOG_ENABLE_THREAD_ID
#endif

#if !defined(ASIO_NET_LOG_DISABLE_DATE_TIME) && !defined(ASIO_NET_LOG_ENABLE_DATE_TIME)
#define ASIO_NET_LOG_ENABLE_DATE_TIME
#endif

#endif
#else
#include <string.h>
#include <stdlib.h>
#endif

#ifdef  ASIO_NET_LOG_LINE_END_CRLF
#define ASIO_NET_LOG_LINE_END            "\r\n"
#else
#define ASIO_NET_LOG_LINE_END            "\n"
#endif

#ifdef ASIO_NET_LOG_NOT_EXIT_ON_FATAL
#define ASIO_NET_LOG_EXIT_PROGRAM()
#else
#ifdef ASIO_NET_LOG_FOR_MCU
#define ASIO_NET_LOG_EXIT_PROGRAM()      do{ for(;;); } while(0)
#else
#define ASIO_NET_LOG_EXIT_PROGRAM()      exit(EXIT_FAILURE)
#endif
#endif

#define ASIO_NET_LOG_BASE_FILENAME       (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define ASIO_NET_LOG_WITH_COLOR

#if defined(_WIN32) || (defined(__ANDROID__) && !defined(ANDROID_STANDALONE)) || defined(ASIO_NET_LOG_FOR_MCU)
#undef ASIO_NET_LOG_WITH_COLOR
#endif

#ifdef ASIO_NET_LOG_DISABLE_COLOR
#undef ASIO_NET_LOG_WITH_COLOR
#endif

#ifdef ASIO_NET_LOG_WITH_COLOR
#define ASIO_NET_LOG_COLOR_RED           "\033[31m"
#define ASIO_NET_LOG_COLOR_GREEN         "\033[32m"
#define ASIO_NET_LOG_COLOR_YELLOW        "\033[33m"
#define ASIO_NET_LOG_COLOR_BLUE          "\033[34m"
#define ASIO_NET_LOG_COLOR_CARMINE       "\033[35m"
#define ASIO_NET_LOG_COLOR_CYAN          "\033[36m"
#define ASIO_NET_LOG_COLOR_DEFAULT
#define ASIO_NET_LOG_COLOR_END           "\033[m"
#else
#define ASIO_NET_LOG_COLOR_RED
#define ASIO_NET_LOG_COLOR_GREEN
#define ASIO_NET_LOG_COLOR_YELLOW
#define ASIO_NET_LOG_COLOR_BLUE
#define ASIO_NET_LOG_COLOR_CARMINE
#define ASIO_NET_LOG_COLOR_CYAN
#define ASIO_NET_LOG_COLOR_DEFAULT
#define ASIO_NET_LOG_COLOR_END
#endif

#define ASIO_NET_LOG_END                 ASIO_NET_LOG_COLOR_END ASIO_NET_LOG_LINE_END

#if defined(__ANDROID__) && !defined(ANDROID_STANDALONE)
#include <android/log.h>
#define ASIO_NET_LOG_PRINTF(...)         __android_log_print(ANDROID_L##OG_DEBUG, "ASIO_NET_LOG", __VA_ARGS__)
#else
#define ASIO_NET_LOG_PRINTF(...)         printf(__VA_ARGS__)
#endif

#ifndef ASIO_NET_LOG_PRINTF_IMPL
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#ifdef ASIO_NET_LOG_ENABLE_THREAD_SAFE
#include <mutex>
struct ASIO_NET_LOG_Mutex {
static std::mutex& mutex() {
static std::mutex mutex;
return mutex;
}
};
#define ASIO_NET_LOG_PRINTF_IMPL(...)    \
std::lock_guard<std::mutex> lock(ASIO_NET_LOG_Mutex::mutex()); \
ASIO_NET_LOG_PRINTF(__VA_ARGS__)
#else
#define ASIO_NET_LOG_PRINTF_IMPL(...)    ASIO_NET_LOG_PRINTF(__VA_ARGS__)
#endif

#else
extern int ASIO_NET_LOG_PRINTF_IMPL(const char *fmt, ...);
#endif

#ifdef ASIO_NET_LOG_ENABLE_THREAD_ID
#include <thread>
#include <sstream>
#include <string>
namespace ASIO_NET_LOG {
inline std::string get_thread_id() {
std::stringstream ss;
ss << std::this_thread::get_id();
return ss.str();
}
}
#define ASIO_NET_LOG_THREAD_LABEL "%s "
#define ASIO_NET_LOG_THREAD_VALUE ,ASIO_NET_LOG::get_thread_id().c_str()
#else
#define ASIO_NET_LOG_THREAD_LABEL
#define ASIO_NET_LOG_THREAD_VALUE
#endif

#ifdef ASIO_NET_LOG_ENABLE_DATE_TIME
#include <chrono>
#include <sstream>
#include <iomanip>
namespace ASIO_NET_LOG {
inline std::string get_time() {
auto now = std::chrono::system_clock::now();
std::time_t time = std::chrono::system_clock::to_time_t(now);
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
std::stringstream ss;
ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
return ss.str();
}
}
#define ASIO_NET_LOG_TIME_LABEL "%s "
#define ASIO_NET_LOG_TIME_VALUE ,ASIO_NET_LOG::get_time().c_str()
#else
#define ASIO_NET_LOG_TIME_LABEL
#define ASIO_NET_LOG_TIME_VALUE
#endif

#define ASIO_NET_LOG(fmt, ...)           do{ ASIO_NET_LOG_PRINTF_IMPL(ASIO_NET_LOG_COLOR_GREEN   ASIO_NET_LOG_TIME_LABEL ASIO_NET_LOG_THREAD_LABEL "[*]: %s:%d "       fmt ASIO_NET_LOG_END ASIO_NET_LOG_TIME_VALUE ASIO_NET_LOG_THREAD_VALUE, ASIO_NET_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define ASIO_NET_LOGT(tag, fmt, ...)     do{ ASIO_NET_LOG_PRINTF_IMPL(ASIO_NET_LOG_COLOR_BLUE    ASIO_NET_LOG_TIME_LABEL ASIO_NET_LOG_THREAD_LABEL "[" tag "]: %s:%d " fmt ASIO_NET_LOG_END ASIO_NET_LOG_TIME_VALUE ASIO_NET_LOG_THREAD_VALUE, ASIO_NET_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define ASIO_NET_LOGI(fmt, ...)          do{ ASIO_NET_LOG_PRINTF_IMPL(ASIO_NET_LOG_COLOR_YELLOW  ASIO_NET_LOG_TIME_LABEL ASIO_NET_LOG_THREAD_LABEL "[I]: %s:%d "       fmt ASIO_NET_LOG_END ASIO_NET_LOG_TIME_VALUE ASIO_NET_LOG_THREAD_VALUE, ASIO_NET_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define ASIO_NET_LOGW(fmt, ...)          do{ ASIO_NET_LOG_PRINTF_IMPL(ASIO_NET_LOG_COLOR_CARMINE ASIO_NET_LOG_TIME_LABEL ASIO_NET_LOG_THREAD_LABEL "[W]: %s:%d [%s] "  fmt ASIO_NET_LOG_END ASIO_NET_LOG_TIME_VALUE ASIO_NET_LOG_THREAD_VALUE, ASIO_NET_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define ASIO_NET_LOGE(fmt, ...)          do{ ASIO_NET_LOG_PRINTF_IMPL(ASIO_NET_LOG_COLOR_RED     ASIO_NET_LOG_TIME_LABEL ASIO_NET_LOG_THREAD_LABEL "[E]: %s:%d [%s] "  fmt ASIO_NET_LOG_END ASIO_NET_LOG_TIME_VALUE ASIO_NET_LOG_THREAD_VALUE, ASIO_NET_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define ASIO_NET_LOGF(fmt, ...)          do{ ASIO_NET_LOG_PRINTF_IMPL(ASIO_NET_LOG_COLOR_CYAN    ASIO_NET_LOG_TIME_LABEL ASIO_NET_LOG_THREAD_LABEL "[!]: %s:%d [%s] "  fmt ASIO_NET_LOG_END ASIO_NET_LOG_TIME_VALUE ASIO_NET_LOG_THREAD_VALUE, ASIO_NET_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); ASIO_NET_LOG_EXIT_PROGRAM(); } while(0) // NOLINT(bugprone-lambda-function-name)

#if defined(ASIO_NET_LOG_IN_LIB) && !defined(ASIO_NET_LOG_SHOW_DEBUG) && !defined(ASIO_NET_LOG_NDEBUG)
#define ASIO_NET_LOG_NDEBUG
#endif

#if defined(NDEBUG) || defined(ASIO_NET_LOG_NDEBUG)
#define ASIO_NET_LOGD(fmt, ...)          ((void)0)
#else
#define ASIO_NET_LOGD(fmt, ...)          do{ ASIO_NET_LOG_PRINTF_IMPL(ASIO_NET_LOG_COLOR_DEFAULT ASIO_NET_LOG_TIME_LABEL ASIO_NET_LOG_THREAD_LABEL "[D]: %s:%d "       fmt ASIO_NET_LOG_END ASIO_NET_LOG_TIME_VALUE ASIO_NET_LOG_THREAD_VALUE, ASIO_NET_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#endif

#if defined(ASIO_NET_LOG_SHOW_VERBOSE)
#define ASIO_NET_LOGV(fmt, ...)          do{ ASIO_NET_LOG_PRINTF_IMPL(ASIO_NET_LOG_COLOR_DEFAULT ASIO_NET_LOG_TIME_LABEL ASIO_NET_LOG_THREAD_LABEL "[V]: %s:%d "       fmt ASIO_NET_LOG_END ASIO_NET_LOG_TIME_VALUE ASIO_NET_LOG_THREAD_VALUE, ASIO_NET_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#else
#define ASIO_NET_LOGV(fmt, ...)          ((void)0)
#endif
