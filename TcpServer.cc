#include "TcpServer.h"
#include <iostream>
#include <netinet/in.h>
#include <string.h>

static EventLoop* checkLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg,
                     Option option)
: loop_(loop)
, ipPort_(listenAddr.toIpPort())
, name_(nameArg)
, acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
, threadPool_(new EventLoopThreadPool(loop_, name_))
, connectionCallback_()
, messageCallback_()
, WriteCompleteCallback_()
, nextConnId_(1)
, started_(0)
{
    // 当有新用户连接时，Acceptor类中绑定的accpetChannel_会有读事件发生，执行handleRead()
    // 调用TcpServer::newConnectionBack_回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

// 有一个新用户连接，acceptor会执行这个回调操作，负责将mainLoop接收到的请求连接
//(acceptChannel_会有读事件发生)通过回调轮询分发给subLoop去处理
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 轮询算法，选择一个subLoop来管理connfd对应的Channel
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, " - %s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;   // 不涉及安全问题，因为只在mainLoop中操作
    std::string conname = name_ + buf;
    LOG_INFO("TcpServer::newConnection[%s] - new connection [%s] from %s\n",
             name_.c_str(),
             conname.c_str(),
             peerAddr.toIpPort().c_str());


    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("TcpConnection::newConnection()::getLocalAddr Error.");
    }

    //
    InetAddress localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop, conname, sockfd, localAddr, peerAddr));

    connections_[conname] = conn;

    // 设置回调
    // 下面的回调都是用户设置给TcpServer=>Tcpconnection=>connectChannel_=>Poller=>notify
    // connectChannel_回调这些函数
    conn->setConnectionCallback(connectionCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(std::bind(removeConnection, this, std::placeholders::_1));

    // 将conn的状态设置为已建立连接，并设置可读，并调用connectionCallback_
    ioLoop->runInLoop(std::bind(&TcpConnection::conectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s.\n",
             name_.c_str(),
             conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}


TcpServer::~TcpServer()
{
    for (auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    num_Threads_ = numThreads;
    threadPool_->setThreadNum(num_Threads_);
}

void TcpServer::start()
{
    if (started_.fetch_add(1) == 0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}