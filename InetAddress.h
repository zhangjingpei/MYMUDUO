#pragma once
#include "copyable.h"
#include <sys/socket.h>

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
class InetAddress : public copyable
{
public:
    // 构造函数，用于初始化InetAddress对象
    explicit InetAddress(uint16_t port=0, std::string ip = "127.0.0.1");

    // 显式构造函数，用于将sockaddr_in结构体转换为InetAddress对象
    explicit InetAddress(const sockaddr_in addr)
    : addr_(addr)
    {}

    std::string toIP() const;       // 将对象表示的IP地址转换为字符串形式
    std::string toIpPort() const;   //
    uint16_t toPort() const;
    // 返回addr_的地址
    const sockaddr_in* getSockAddr() const { return &addr_; }

    // 设置sockaddr_in类型的地址
    void setSockAddr(const sockaddr_in& addr) { addr_ = addr; }


    // 定义一个私有的sockaddr_in结构体，用于存储IPv4地址
private:
    struct sockaddr_in addr_;
};