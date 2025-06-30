/*
用户使用muduo库编写服务器程序
*/
#include "Acceptor.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "logging.h"
#include "noncopyable.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class InetAddress;
class EventLoop;
class Acceptor;
class EventLoopThreadPool;


// 对外的服务器编程使用的类
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };
    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb)
    {
        threadInitCallback_ = std::move(cb);
    }
    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connectionCallback_ = std::move(cb);
    }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        WriteCompleteCallback_ = std::move(cb);
    }

    // 设置底层subLoop的数量
    void setThreadNum(int numThreads);

    /*
    如果没有监听，就启动服务器
    多次调动没有副作用
    线程安全
    */
    void start();


private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& connr);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_;   // baseLoop自定义的loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;   // 运行在mainloop 任务就是监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_;   // 为什么是shared_ptr

    ConnectionCallback connectionCallback_;         // 有连接时的回调
    MessageCallback messageCallback_;               // 有读写事件发生时的回调
    WriteCompleteCallback WriteCompleteCallback_;   // 消息发送完成后的回调

    ThreadInitCallback threadInitCallback_;   // loop线程初始化时的回调

    int num_Threads_;           // 线程数量
    std::atomic_int started_;   //

    int nextConnId_;
    ConnectionMap connections_;
};