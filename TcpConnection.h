#pragma once

#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include <atomic>
#include <functional>
#include <memory>
#include <queue>


class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd==>设置为TcpConnection类型
 * => TcpConnection设置回调 => 设置到Channel => Poller => Channel回调
 **/

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                  const InetAddress& localAddr, const InetAddress& peerAddr);

    ~TcpConnection();

    std::string name() { return name_; }
    EventLoop* getLoop() { return loop_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_.load() == kConnected; }

    void send(const std::string& buf);   // 发送数据
    void sendFile(int fileDescriptor, off_t offset, size_t count);
    void shutdown();   // 关闭当前连接

    void setConnectionCallback(ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(WriteCompleteCallback& cb)
    {
        writeCompleteCallback_ = cb;
    }
    void setHighWaterMarkCallback(HighWaterMarkCallback& cb)
    {
        highWaterMarkCallback_ = cb;
    }
    void setCloseCallback(CloseCallback& cb) { closeCallback_ = cb; }

    void coonectEstablished();
    void connectDestroyed();

private:
    enum StateE
    {
        kDisconnected,   // 已经断开连接
        kConnecting,     // 正在连接
        kConnected,      // 已连接
        kDisConnecting   // 正在断开连接
    };

    struct FileBlock
    {
        int fd_;
        off_t offset;
        ssize_t remaining;
    };

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();
    void sendFileInLoop(int fileDescriptor, off_t offset, size_t count);

    void setState(StateE s) { state_.store(s); }
    const char* stateToString() const;



    EventLoop*
        loop_;   // 这里绝对不是baseLoop,因为TcpConnection都是在subLoop中管理的，mainLoop通过轮询得出的ioLoop
    const std::string name_;
    std::atomic<StateE> state_;   //?
    bool reading_;

    // 这里和Acceptor相似 Acceptor->mainLoop     TcpConnection->subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> connectchannel_;   // 将connfd和ioLoop绑定到Channel中
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;   // 水位线标志

    Buffer inputBuffer_;    // 存储从 TCP 套接字接收到的数据（来自对端的数据）。
    Buffer outputBuffer_;   // 存储待发送给对端的数据。

    std::queue<FileBlock> fileQueue_;   // 文件状态
};