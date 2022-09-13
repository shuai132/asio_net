/**
* 统一控制调试信息
* 为了保证输出顺序 都使用stdout而不是stderr
*
* 可配置项（默认都是未定义）
* asio_net_LOG_SHOW_DEBUG           开启LOGD的输出
* asio_net_LOG_SHOW_VERBOSE         显示LOGV的输出
* asio_net_LOG_DISABLE_COLOR        禁用颜色显示
* asio_net_LOG_LINE_END_CRLF        默认是\n结尾 添加此宏将以\r\n结尾
* asio_net_LOG_FOR_MCU              MCU项目可配置此宏 更适用于MCU环境
* asio_net_LOG_NOT_EXIT_ON_FATAL    FATAL默认退出程序 添加此宏将不退出
*
* 其他配置项
* asio_net_LOG_PRINTF_IMPL          定义输出实现（默认使用printf）
* 并添加形如int asio_net_LOG_PRINTF_IMPL(const char *fmt, ...)的实现
*/

#pragma once

// clang-format off

#ifdef __cplusplus
#include <cstring>
#include <cstdlib>
#else
#include <string.h>
#include <stdlib.h>
#endif

#ifdef  asio_net_LOG_LINE_END_CRLF
#define asio_net_LOG_LINE_END            "\r\n"
#else
#define asio_net_LOG_LINE_END            "\n"
#endif

#ifdef asio_net_LOG_NOT_EXIT_ON_FATAL
#define asio_net_LOG_EXIT_PROGRAM()
#else
#ifdef asio_net_LOG_FOR_MCU
#define asio_net_LOG_EXIT_PROGRAM()      do{ for(;;); } while(0)
#else
#define asio_net_LOG_EXIT_PROGRAM()      exit(EXIT_FAILURE)
#endif
#endif

#define asio_net_LOG_BASE_FILENAME       (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                                       strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define asio_net_LOG_WITH_COLOR

#if defined(_WIN32) || defined(__ANDROID__) || defined(asio_net_LOG_FOR_MCU)
#undef asio_net_LOG_WITH_COLOR
#endif

#ifdef asio_net_LOG_DISABLE_COLOR
#undef asio_net_LOG_WITH_COLOR
#endif

#ifdef asio_net_LOG_WITH_COLOR
#define asio_net_LOG_COLOR_RED           "\033[31m"
#define asio_net_LOG_COLOR_GREEN         "\033[32m"
#define asio_net_LOG_COLOR_YELLOW        "\033[33m"
#define asio_net_LOG_COLOR_BLUE          "\033[34m"
#define asio_net_LOG_COLOR_CARMINE       "\033[35m"
#define asio_net_LOG_COLOR_CYAN          "\033[36m"
#define asio_net_LOG_COLOR_DEFAULT
#define asio_net_LOG_COLOR_END           "\033[m"
#else
#define asio_net_LOG_COLOR_RED
#define asio_net_LOG_COLOR_GREEN
#define asio_net_LOG_COLOR_YELLOW
#define asio_net_LOG_COLOR_BLUE
#define asio_net_LOG_COLOR_CARMINE
#define asio_net_LOG_COLOR_CYAN
#define asio_net_LOG_COLOR_DEFAULT
#define asio_net_LOG_COLOR_END
#endif

#define asio_net_LOG_END                 asio_net_LOG_COLOR_END asio_net_LOG_LINE_END

#if __ANDROID__
#include <android/log.h>
#define asio_net_LOG_PRINTF(...)         __android_log_print(ANDROID_asio_net_LOG_DEBUG, "LOG", __VA_ARGS__)
#else
#define asio_net_LOG_PRINTF(...)         printf(__VA_ARGS__)
#endif

#ifndef asio_net_LOG_PRINTF_IMPL
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif
#define asio_net_LOG_PRINTF_IMPL(...)    asio_net_LOG_PRINTF(__VA_ARGS__) // NOLINT(bugprone-lambda-function-name)
#else
extern int asio_net_LOG_PRINTF_IMPL(const char *fmt, ...);
#endif

#define asio_net_LOG(fmt, ...)           do{ asio_net_LOG_PRINTF_IMPL(asio_net_LOG_COLOR_GREEN "[*]: " fmt asio_net_LOG_END, ##__VA_ARGS__); } while(0)

#define asio_net_LOGT(tag, fmt, ...)     do{ asio_net_LOG_PRINTF_IMPL(asio_net_LOG_COLOR_BLUE "[" tag "]: " fmt asio_net_LOG_END, ##__VA_ARGS__); } while(0)
#define asio_net_LOGI(fmt, ...)          do{ asio_net_LOG_PRINTF_IMPL(asio_net_LOG_COLOR_YELLOW "[I]: %s: " fmt asio_net_LOG_END, asio_net_LOG_BASE_FILENAME, ##__VA_ARGS__); } while(0)
#define asio_net_LOGW(fmt, ...)          do{ asio_net_LOG_PRINTF_IMPL(asio_net_LOG_COLOR_CARMINE "[W]: %s: %s: %d: " fmt asio_net_LOG_END, \
                                           asio_net_LOG_BASE_FILENAME, __func__, __LINE__, ##__VA_ARGS__); \
                                       } while(0)
#define asio_net_LOGE(fmt, ...)          do{ asio_net_LOG_PRINTF_IMPL(asio_net_LOG_COLOR_RED "[E]: %s: %s: %d: " fmt asio_net_LOG_END, \
                                           asio_net_LOG_BASE_FILENAME, __func__, __LINE__, ##__VA_ARGS__); \
                                       } while(0)
#define asio_net_FATAL(fmt, ...)         do{ asio_net_LOG_PRINTF_IMPL(asio_net_LOG_COLOR_CYAN "[!]: %s: %s: %d: " fmt asio_net_LOG_END, \
                                           asio_net_LOG_BASE_FILENAME, __func__, __LINE__, ##__VA_ARGS__); \
                                           asio_net_LOG_EXIT_PROGRAM(); \
                                       } while(0)

#if defined(asio_net_LOG_SHOW_DEBUG)
#define asio_net_LOGD(fmt, ...)          do{ asio_net_LOG_PRINTF_IMPL(asio_net_LOG_COLOR_DEFAULT "[D]: %s: " fmt asio_net_LOG_END, asio_net_LOG_BASE_FILENAME, ##__VA_ARGS__); } while(0)
#else
#define asio_net_LOGD(fmt, ...)          ((void)0)
#endif

#if defined(asio_net_LOG_SHOW_VERBOSE)
#define asio_net_LOGV(fmt, ...)          do{ asio_net_LOG_PRINTF_IMPL(asio_net_LOG_COLOR_DEFAULT "[V]: %s: " fmt asio_net_LOG_END, asio_net_LOG_BASE_FILENAME, ##__VA_ARGS__); } while(0)
#else
#define asio_net_LOGV(fmt, ...)          ((void)0)
#endif
