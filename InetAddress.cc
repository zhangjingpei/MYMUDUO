#include"InetAddress.h"
#include <cstring>


// 构造函数，用于初始化InetAddress对象
InetAddress::InetAddress(uint16_t port, std::string ip)
{
    // 将addr_结构体中的所有字节设置为0
    ::memset(&addr_,0,sizeof(addr_));
    // 将端口号转换为网络字节序
    addr_.sin_port = ::htons(port);
    // 设置地址族为IPv4
    addr_.sin_family = AF_INET;
    // 将IP地址转换为网络字节序
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());

}

// 将InetAddress对象转换为IP地址字符串
std::string InetAddress::toIP() const
{
    // 定义一个字符数组，用于存储IP地址字符串
    char buf[64]={0};
    // 使用inet_ntop函数将addr_中的IP地址转换为字符串，并存储在buf中
    ::inet_ntop(AF_INET,&addr_.sin_addr, buf, sizeof(buf));
    // 返回IP地址字符串
    return std::string(buf);
}

// 将InetAddress对象转换为IP地址和端口号的字符串表示
std::string InetAddress::toIpPort() const
{
    // 定义一个长度为64的字符数组，用于存储IP地址和端口号
    char buf[64] = {0};

    // 将InetAddress对象的IP地址转换为字符串，并存储在buf中
    ::inet_ntop(AF_INET,&addr_.sin_addr, buf, sizeof(buf));

    // 获取buf中IP地址的长度
    size_t end = ::strlen(buf);
    buf[end] = ':';
    // 将InetAddress对象的端口号转换为本地字节序（小端序）
    uint16_t port = ::ntohs(addr_.sin_port);
    
    // 将端口号转换为字符串，并存储在buf中
    sprintf(buf+end+1,"%u",port);

    // 返回IP地址和端口号的字符串表示
    return buf;
}
// 将InetAddress对象的端口号转换为网络字节序
uint16_t InetAddress::toPort() const
{
    // 使用ntohs函数将addr_对象的sin_port字段从网络字节序转换为主机字节序
    return ::ntohs(addr_.sin_port);
}

// #include <iostream>
// int main()
// {
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;
// }
