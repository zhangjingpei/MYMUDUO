#pragma once

#include "noncopyable.h"
#include <iostream>
#include <stdio.h>
#include <string>

// LOG_INFO("%s,%d",arg1,arg2)
#define LOG_INFO(LogmsgFormat, ...)                              \
    do {                                                         \
        Logger& logger = Logger::instance();                     \
        logger.setLogLevel(INFO);                                \
        char buf[1024] = {0};                                    \
        snprintf(buf, sizeof(buf), LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                         \
    } while (0)
#define LOG_ERROR(LogmsgFormat, ...)                             \
    do {                                                         \
        Logger& logger = Logger::instance();                     \
        logger.setLogLevel(ERROR);                               \
        char buf[1024] = {0};                                    \
        snprintf(buf, sizeof(buf), LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                         \
    } while (0)

#define LOG_FATAL(LogmsgFormat, ...)                             \
    do {                                                         \
        Logger& logger = Logger::instance();                     \
        logger.setLogLevel(FATAL);                               \
        char buf[1024] = {0};                                    \
        snprintf(buf, sizeof(buf), LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                         \
    } while (0)

#ifdef MUDEBUG
#    define LOG_DEBUG(LogmsgFormat, ...)                             \
        do {                                                         \
            Logger& logger = Logger::instance();                     \
            logger.setLogLevel(DEBUG);                               \
            char buf[1024] = {0};                                    \
            snprintf(buf, sizeof(buf), LogmsgFormat, ##__VA_ARGS__); \
            logger.log(buf);                                         \
        } while (0)
#else
#    define LOG_DEBUG(LogmsgFormat, ...)
#endif

enum LogLevel
{
    INFO,  /**< 普通信息，用于记录一般的信息。 */
    ERROR, /**< 错误信息，用于记录错误情况。注意：原代码中可能有拼写错误，应为ERROR。 */
    FATAL, /**< 严重错误信息，用于记录导致程序崩溃或严重故障的信息。 */
    DEBUG  /**< 调试信息，用于记录调试过程中需要的信息。 */
};

// 输出一个日志类
// 定义一个Logger类
class Logger : noncopyable   // 单例模式
{
public:
    static Logger& instance();     // 获取Logger实例的指针
    void setLogLevel(int level);   // 设置日志级别
    void log(std::string msg);     // 写日志
private:
    int logLevel_;       // 日志级别
    Logger() = default;   // 构造函数
};
// info
