#include "Timestamp.h"
#include "logging.h"
// 获取日志唯一的实例对象
Logger& Logger::instance()
{
    static Logger logger;   // 为什么是线程安全的
    /*
    即使多个线程同时调用 Logger::instance()，
    保证只有一个线程能够初始化这个静态变量，
    其他线程将被阻塞直到初始化完成。
    c++ 11 在实现局部静态变量的初始化时，会使用互斥机制（例如互斥锁），来实现线程安全
    */

    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
    // 将传入的日志级别赋值给logLevel_成员变量
    logLevel_ = level;
}

void Logger::log(std::string msg)
{
    switch (logLevel_) {
    case INFO: std::cout << "[INFO]:"; break;
    case ERROR: std::cout << "[ERROR]:";
    case FATAL: std::cout << "[FATAL]:";
    case DEBUG: std::cout << "[DEBUG]:";
    default: break;
    }
    std::cout << Timestamp::now().toString() << ":" << msg << std::endl;
}
