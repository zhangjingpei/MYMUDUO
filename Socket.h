#pragma once

#include "noncopyable.h"

class InetAddress;

// 负责监听端口的监听、绑定、接收
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
    : sockfd_(sockfd)   // 监听fd由外部生成
    {}
    ~Socket();
    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress& localaddr);

    void listen();

    int accept(InetAddress* peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);   // Nagle算法
    void setReuseAddr(bool on);    // 地址复用
    void setReusePort(bool on);    // 端口复用
    void setKeepAlive(bool on);    // TCP保活机制

private:
    const int sockfd_;   // 等同于listernfd_，监听fd
};