#include "Socket.h"
#include "InetAddress.h"
#include "logging.h"
#include <string.h>   //memset
// #include<unistd.h>
// #include<sys/types.h>
// #include<sys/socket.h>
#include <netinet/tcp.h>   //TCP_NODELAY
#include <unistd.h>        //close bind



Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr)
{
    if (0 != ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("Socket::bindAddress -- sockfd:_%d failed.\n", sockfd_);
    }
}

void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d failed.\n", sockfd_);
    }
}

//接收新连接，并将地址、端口写入到peeraddr
int Socket::accept(InetAddress* peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd > 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;//客户端套接字fd
}

// 关闭监听端口
// SHUT_WR表示关闭套接字的写端(不再发送数据)、但是保存读端(能够接收数据)
void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR))
    {
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)   // Nagle算法
{
    // TcpNoDelat用于禁用Nagle算法
    // Nagle算法用于减少网络上传输的小数据包数量
    // 将TCP_NODELAY设置为1可以禁用该算法、允许小数据包立即发送
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
void Socket::setReuseAddr(bool on)   // 地址复用
{
    // SO_REUSEADDR允许一个套接字强制绑定到一个已经被其他套接字使用的端口
    // 这对于需要重启并绑定到相同端口的服务器应用程序非常有用
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)   // 端口复用
{
    // SO_REUSEPORT允许用一个主机上的多个套接字绑定到相同的端口
    // 这对于在多个线程或进程之间负载均衡传入连接非常有用
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}


void Socket::setKeepAlive(bool on)   // TCP保活机制
{
    // SO_KEEPALIVE启用在已连接的套接字上定期传输消息
    // 如果另一端没有响应，则认为连接已经关闭
    // 这对于检测网络中失效的对等方非常有用
    //当 TCP 保活探测失败（对端长时间无响应）, 内核会主动关闭这个 socket 连接。
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}
