#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "logging.h"

#include <cassert>
#include <errno.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>
// 检查loop指针是否为空，若为空则打印致命日志并终止程序
static EventLoop* checkLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

// TcpConnection构造函数，初始化各成员变量，并设置Channel的回调
TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                             const InetAddress& localAddr, const InetAddress& peerAddr)
: loop_(checkLoopNotNull(loop))
, name_(name)
, state_(kConnecting)
, reading_(true)
, socket_(new Socket(sockfd))                  // 用来关闭套接字的写功能以及保活机制
, connectchannel_(new Channel(loop, sockfd))   // 相当于Acceptor中的AcceptChannel
, localAddr_(localAddr)
, peerAddr_(peerAddr)
, highWaterMark_(64 * 1024 * 1024)   // 64M
{
    // 设置Channel的各种事件回调
    connectchannel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    connectchannel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    connectchannel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    connectchannel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnction::ctor[%s] at %p, fd = %d.\n", name_.c_str(), this, sockfd);

    socket_->setKeepAlive(true);
}

// 析构函数，打印日志
TcpConnection::~TcpConnection()
{
    // 清理未发送完成的文件
    while (!fileQueue_.empty())
    {
        ::close(fileQueue_.front().fd_);
        fileQueue_.pop();
    }
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d.\n",
             name_.c_str(),
             connectchannel_->fd(),
             state_.load());
}

// 处理可读事件
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(connectchannel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 调用用户设置的消息回调
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        // 对端关闭连接
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead()");
        handleError();
    }
}

// 处理可写事件
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (connectchannel_->isWriting() && outputBuffer_.readableBytes() > 0)   // 如果关注的是写事件
    {
        int savedError = 0;
        // 从outputBuffer_中的可读区域向内核写n个数据
        ssize_t n = outputBuffer_.sendFd(connectchannel_->fd(), &savedError);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
            if (state_.load() == kDisConnecting)
            {
                shutdownInLoop();
            }
        }
    }
    else if (outputBuffer_.readableBytes() == 0 && !fileQueue_.empty())
    {
        FileBlock& file = fileQueue_.front();
        ssize_t n = ::sendfile(connectchannel_->fd(), file.fd_, &file.offset, file.remaining);
        if (n > 0)
        {
            file.remaining -= n;
            if (file.remaining == 0)
            {
                ::close(file.fd_);
                fileQueue_.pop();

                if (fileQueue_.empty() && writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
        }
        else if (n < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::handleWrite(sendfile).\n");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    handleClose();
                }
            }
        }
    }
    else if (outputBuffer_.readableBytes() == 0 && fileQueue_.empty())
    {
        // 所有数据发送完成，禁用写事件
        if (connectchannel_->isWriting())
        {
            connectchannel_->disableWriting();
        }

        // 触发写完成回调
        if (writeCompleteCallback_)
        {
            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }

        // 如果正在断开连接，关闭写端
        if (state_.load() == kDisConnecting)
        {
            shutdownInLoop();
        }
    }
    else if (outputBuffer_.readableBytes() > 0 || !fileQueue_.empty())
    {
        if (!connectchannel_->isWriting())
        {
            connectchannel_->enableWriting();
        }
    }
    else
    {
        LOG_ERROR("Connction fd = %d is down, no more writing.\n", connectchannel_->fd());
    }
}


// 处理连接关闭事件
void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_INFO("fd = %d, state = %s", connectchannel_->fd(), stateToString());

    assert(state_.load() == kConnected || state_.load() == kDisConnecting);
    setState(kDisconnected);

    // 清理未发送完的文件
    while (!fileQueue_.empty())
    {
        ::close(fileQueue_.front().fd_);
        fileQueue_.pop();
    }

    connectchannel_->disableAll();


    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);   // 执行连接关闭的回调

    if (closeCallback_) closeCallback_(guardThis);   // 关闭连接的回调
}

// 处理错误事件
void TcpConnection::handleError()
{
    int err;
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(connectchannel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError() [%s] - SO_ERROR = %d.\n", name_.c_str(), err);
}

// 发送数据（线程安全）
void TcpConnection::send(const std::string& buf)
{
    if (state_.load() == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            // 不是在IO线程则投递到IO线程执行
            loop_->queueInLoop(
                std::bind(&TcpConnection::sendInLoop, shared_from_this(), buf.c_str(), buf.size()));
        }
    }
}

/*
发送数据 应用写的块，而内核发送数据慢，需要把待发送数据写入缓冲区，而且设置了水位回调
*/
/*
工作流程简述
1.注册写事件
当你有数据要发送，但内核缓冲区满了，write 返回 EWOULDBLOCK，此时需要通过 Channel
注册写事件（EPOLLOUT），让 epoll 关注这个 fd 的可写状态。

2.内核通知可写
当内核缓冲区有空间时，epoll 会通知应用层“写事件就绪”，即调用 handleWrite()。

3.应用层写数据
在 handleWrite() 里，把 outputBuffer_ 里的数据写到
fd。如果数据全部写完，就可以取消写事件关注（disableWriting()），避免无意义的 epoll 触发。

4.写事件的意义

避免 busy loop：只有在不能立即写完时才注册写事件，等内核通知再写。
保证数据顺序和完整性：只有 outputBuffer_ 为空且未关注写事件时，才直接
write，防止乱序和重复注册。

*/
void TcpConnection::sendInLoop(const void* message, size_t len)
{
    ssize_t nwrite = 0;        // 已经写的数据长度
    ssize_t remaining = len;   // 剩余的数据长度
    bool falutError = false;

    // 之前调用过该connection的shutdown，不能再进行发送了
    if (state_.load() == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing.\n ");
        return;
    }

    // 只有当当前Channel没有关注写事件且outputBuffer_没有待发送数据时，才尝试直接写socket
    // 为什么要判断!connectchannel_->isWriting()？
    // 因为如果Channel已经关注写事件，说明之前有数据没写完，内核缓冲区满了，必须等epoll通知可写再写。
    // 如果此时直接write，可能会导致数据乱序或重复注册写事件。
    // 只有在没有关注写事件且缓冲区为空时，才可以直接write尝试快速发送，提高效率。
    if (!connectchannel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        // ::write不是写到
        // outputBuffer_，而是直接尝试发送到对端。如果内核缓冲区满了，才会把剩余数据 append 到
        // outputBuffer_，并注册写事件，等待下次可写时再发送。
        nwrite = ::write(connectchannel_->fd(), message, len);

        if (nwrite > 0)
        {
            remaining = len - nwrite;
            // 数据全部发送完成，直接回调写完成
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else   // nwrite <= 0
        {
            nwrite = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop.\n");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    falutError = true;
                }
            }
        }
    }   // end if !connectchannel_->isWriting()

    // falutError=false且remaining>0
    // 说明当前这一次write并没有将数据全部发送出去，需要将其保存到outputBuffer_中，然后connectchannel_注册epollout事件，
    // poller发现内核缓冲区空余，则会通知connectchannel_，调用handlewrite()方法
    if (!falutError && remaining > 0)
    {
        // 目前缓冲区剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();

        // 说明本次 append 之后，outputBuffer_ 里的数据总量将达到或超过高水位线
        // 说明append 之前还没达到高水位线，只有这次 append
        // 才刚好“越过”高水位线，只触发一次回调。 用户确实设置了高水位回调
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ &&
            highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }

        outputBuffer_.append((char*)message + nwrite, remaining);
        if (!connectchannel_->isWriting())
        {
            connectchannel_->enableWriting();
        }
    }
}

// 主动关闭连接
void TcpConnection::shutdown()
{
    if (state_.load() == kConnected)
    {
        setState(kDisConnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop()
{

    if (!connectchannel_->isWriting() && fileQueue_.empty())
    {
        socket_->shutdownWrite();   // 服务器关闭写端之后 若对端也关闭连接，handleClose
                                    // 会被调用，触发用户定义的连接关闭回调（connectionCallback_ 和
                                    // closeCallback_）
    }
}

void TcpConnection::coonectEstablished()
{
    setState(kConnected);
    connectchannel_->tie(
        shared_from_this());   // 确保在 Channel 处理 I/O 事件时，TcpConnection 对象不会被提前析构
    connectchannel_->enableReading();

    // 新连接建立执行回调
    connectionCallback_(shared_from_this());
}


void TcpConnection::connectDestroyed()
{
    if (state_.load() == kConnected || state_.load() == kDisConnecting)
    {
        setState(kDisconnected);
        connectchannel_->disableAll();
        connectionCallback_(shared_from_this());
    }

    connectchannel_->remove();   // 将connectchannel_从poller中删除掉
}

const char* TcpConnection::stateToString() const
{
    switch (state_.load())
    {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisConnecting:
            return "kDisConnecting";
        default:
            return "unknown state";
    }
}

void TcpConnection::sendFile(int fileDescriptor, off_t offset, size_t count)
{
    if (connected())
    {
        if (loop_->isInLoopThread())
        {
            sendFileInLoop(fileDescriptor, offset, count);
        }
        else
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, count));
        }
    }
    else
    {
        LOG_ERROR("TcpConnection::sendFile - not connceted.\n");
        ::close(fileDescriptor);
    }
}

// 在事件循环中执行sendFileInLoop
void TcpConnection::sendFileInLoop(int fileDescriptor, off_t offset, size_t count)
{
    loop_->assertInLoopThread();
    ssize_t bytesSent = 0;       // 发送了多少字节
    ssize_t remaining = count;   // 还有多少字节待发送
    bool faultError = false;     // 是否发生错误

    if (state_.load() == kDisConnecting)
    {
        LOG_ERROR("TcpConnection::disconnected, give up writing.\n");
        ::close(fileDescriptor);   // 关闭文件描述符
        return;
    }

    // 如果当前没有待发送数据，尝试直接发送
    if (!connectchannel_->isWriting() && outputBuffer_.readableBytes() == 0 && fileQueue_.empty())
    {
        // 将fileDescriptor套接字的数据发送到connectfd上
        bytesSent = ::sendfile(socket_->fd(), fileDescriptor, &offset, remaining);
        if (bytesSent > 0)
        {
            remaining -= bytesSent;
            if (remaining == 0)
            {
                ::close(fileDescriptor);
                if (writeCompleteCallback_)
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            bytesSent = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::senfFileInLoop().\n");
                if (errno == EPIPE || errno == ECONNREFUSED)
                {
                    faultError = true;
                }
                ::close(fileDescriptor);   // 发生错误，关闭文件
            }
        }
    }

    // 处理剩余数据
    // 未发送完且无错误 -> 注册写事件
    if (!faultError && remaining > 0)
    {

        fileQueue_.push({fileDescriptor, offset, remaining});


        // 高水位检查
        if (fileQueue_.size() > 1 && highWaterMarkCallback_)
        {
            size_t queueSize = fileQueue_.size();
            loop_->queueInLoop(std::bind(
                highWaterMarkCallback_, shared_from_this(), queueSize * sizeof(FileBlock)));
        }
        if (!connectchannel_->isWriting())
        {
            connectchannel_->enableWriting();
        }
    }
}