/**
 * 统一控制调试信息
 * 为了保证输出顺序 都使用stdout而不是stderr
 *
 * 可配置项（默认都是未定义）
 * asio_net_LOG_LINE_END_CRLF        默认是\n结尾 添加此宏将以\r\n结尾
 * asio_net_LOG_SHOW_DEBUG           开启LOGD的输出
 * asio_net_LOG_SHOW_VERBOSE         显示LOGV的输出
 */

#pragma once

#include <stdio.h>

// clang-format off

#ifdef  asio_net_LOG_LINE_END_CRLF
#define asio_net_LOG_LINE_END        "\r\n"
#else
#define asio_net_LOG_LINE_END        "\n"
#endif

#ifdef asio_net_LOG_NOT_EXIT_ON_FATAL
#define asio_net_LOG_EXIT_PROGRAM()
#else
#ifdef asio_net_LOG_FOR_MCU
#define asio_net_LOG_EXIT_PROGRAM()  do{ for(;;); } while(0)
#else
#define asio_net_LOG_EXIT_PROGRAM()  exit(EXIT_FAILURE)
#endif
#endif

#ifdef asio_net_LOG_WITH_COLOR
#define asio_net_LOG_COLOR_RED       "\033[31m"
#define asio_net_LOG_COLOR_GREEN     "\033[32m"
#define asio_net_LOG_COLOR_YELLOW    "\033[33m"
#define asio_net_LOG_COLOR_BLUE      "\033[34m"
#define asio_net_LOG_COLOR_CARMINE   "\033[35m"
#define asio_net_LOG_COLOR_CYAN      "\033[36m"
#define asio_net_LOG_COLOR_END       "\033[m"
#else
#define asio_net_LOG_COLOR_RED
#define asio_net_LOG_COLOR_GREEN
#define asio_net_LOG_COLOR_YELLOW
#define asio_net_LOG_COLOR_BLUE
#define asio_net_LOG_COLOR_CARMINE
#define asio_net_LOG_COLOR_CYAN
#define asio_net_LOG_COLOR_END
#endif

#define asio_net_LOG_END             asio_net_LOG_COLOR_END asio_net_LOG_LINE_END

#ifdef __ANDROID__
#include <android/log.h>
#define asio_net_LOG_PRINTF(...)     __android_log_print(ANDROID_LOG_DEBUG, "LOG", __VA_ARGS__)
#else
#define asio_net_LOG_PRINTF(...)     printf(__VA_ARGS__)
#endif

#if defined(asio_net_LOG_SHOW_VERBOSE)
#define asio_net_LOGV(fmt, ...)      do{ asio_net_LOG_PRINTF("[V]: " fmt asio_net_LOG_LINE_END, ##__VA_ARGS__); } while(0)
#else
#define asio_net_LOGV(fmt, ...)      ((void)0)
#endif

#if defined(asio_net_LOG_SHOW_DEBUG)
#define asio_net_LOGD(fmt, ...)      do{ asio_net_LOG_PRINTF("[D]: " fmt asio_net_LOG_LINE_END, ##__VA_ARGS__); } while(0)
#else
#define asio_net_LOGD(fmt, ...)      ((void)0)
#endif

#define asio_net_LOG(fmt, ...)       do{ asio_net_LOG_PRINTF(asio_net_LOG_COLOR_GREEN fmt asio_net_LOG_END, ##__VA_ARGS__); } while(0)
#define asio_net_LOGT(tag, fmt, ...) do{ asio_net_LOG_PRINTF(asio_net_LOG_COLOR_BLUE "[" tag "]: " fmt asio_net_LOG_END, ##__VA_ARGS__); } while(0)
#define asio_net_LOGI(fmt, ...)      do{ asio_net_LOG_PRINTF(asio_net_LOG_COLOR_YELLOW "[I]: " fmt asio_net_LOG_END, ##__VA_ARGS__); } while(0)
#define asio_net_LOGW(fmt, ...)      do{ asio_net_LOG_PRINTF(asio_net_LOG_COLOR_CARMINE "[W]: %s: %d: " fmt asio_net_LOG_END, __func__, __LINE__, ##__VA_ARGS__); } while(0)
#define asio_net_LOGE(fmt, ...)      do{ asio_net_LOG_PRINTF(asio_net_LOG_COLOR_RED "[E]: %s: %d: " fmt asio_net_LOG_END, __func__, __LINE__, ##__VA_ARGS__); } while(0)
