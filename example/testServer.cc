#include <string>

#include <mymuduo/TcpServer.h>
#include <mymuduo/logging.h>

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
    : server_(loop, addr, name)
    , loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        server_.setMessageCallback(std::bind(&EchoServer::onMessage,
                                             this,
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3));

        // 设置合适的subloop线程数量
        server_.setThreadNum(3);
    }
    void start() { server_.start(); }

private:
    // 连接建立或断开的回调函数
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }
    TcpServer server_;
    EventLoop* loop_;
};

int main()
{
    EventLoop loop;
    // std::cout << "main::baseloop_: " << loop.getId() << std::endl;
    InetAddress addr(8080);
    // std::cout << "main::InetAddress: " << addr.toIpPort() << std::endl;
    EchoServer server(&loop, addr, "EchoServer");
    server.start();
    loop.loop();//启动mainLoop的底层Poller
    return 0;
}