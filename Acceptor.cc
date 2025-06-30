#include "Acceptor.h"
#include "InetAddress.h"
#include "logging.h"

#include <errno.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// 创建非阻塞的监听fd
static int createNonBlocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d.\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
: loop_(loop)
, acceptSocket_(createNonBlocking())
, acceptChannel_(loop, acceptSocket_.fd())
, listenning_(false)
{
    acceptSocket_.setReuseAddr(reuseport);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);

    // TcpServer::start() =>Acceptor.listen() 如果有新用户连接，
    // 要执行一个回调(accept => connfd => 打包成Channel => 唤醒subLoop)
    // baseLoop_监听到有事件发生 => acceptChannel_(listenfd) =》 执行回调函数
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

// listenfd有事件发生了，也就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd, peerAddr);   // !!进入loop循环
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();   // 从Poller中将感兴趣的事件移除掉

    // 调用EventLoop->removeChannel => Poller::removeChannel()从Channel列表中移除
    // 即把Poller中的ChannelMap对应的部分删除。
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();           // 打开监听
    acceptChannel_.enableReading();   // 向Poller注册感兴趣的事件为读。!! 重要
}
