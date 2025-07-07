#pragma once
#include "Channel.h"
#include "Socket.h"
#include "noncopyable.h"
#include <functional>


class EventLoop;
class InetAddress;


// 负责调度
class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();
    // 设置新连接的回调函数
    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        NewConnectionCallback_ = cb;
    }
    // 判断是否在监听
    bool listening() { return listenning_; }
    // 监听本地端口
    void listen();

private:
    void handleRead();   // 处理用户的连接请求

    EventLoop* loop_;                               // Acceptor用的是Pool中的baseloop_;
    Socket acceptSocket_;                           // 专门用于接收新连接的socket
    Channel acceptChannel_;                         // 专门用于监听新连接的Channel
    NewConnectionCallback NewConnectionCallback_;   // 新连接的回调函数
    bool listenning_;                               // 是否在监听
};