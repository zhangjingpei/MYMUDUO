#include <fcntl.h>
#include <mymuduo/TcpServer.h>
#include <mymuduo/logging.h>
#include <string>
#include <unistd.h>

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
        server_.setThreadNum(10);
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
    // void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    // {
    //     std::string msg = buf->retrieveAllAsString();
    //     conn->send(msg);
    //     conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    // }
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO("Received message: %s", msg.c_str());

        if (msg.find("SEND_FILE") == 0)
        {
            // 打开要发送的文件
            const char* filename = "testfile.txt";
            int fd = ::open(filename, O_RDONLY);
            if (fd < 0)
            {
                LOG_ERROR("Failed to open file %s", filename);
                conn->send("FILE_NOT_FOUND");
                return;
            }
            LOG_INFO("go on Mesage::send_file.\n");
            // 获取文件大小
            off_t fileSize = lseek(fd, 0, SEEK_END);
            lseek(fd, 0, SEEK_SET);

            // 发送文件
            conn->send("SENDING_FILE\r\n");
            conn->sendFile(fd, 0, fileSize);
            LOG_INFO("Sending file %s (size=%ld) to client", filename, fileSize);
        }
        else
        {
            // 普通回显
            conn->send("ECHO: " + msg);
        }
    }
    TcpServer server_;
    EventLoop* loop_;
};

int main()
{
    FILE* fp = fopen("testfile.txt", "w");
    if (fp)
    {
        fprintf(fp, "This is a test file sent via sendfile system call.\n");
        fprintf(fp, "This file demonstrates the zero-copy file transfer capability.\n");
        fprintf(fp, "File contents generated at server startup.\n");
        fclose(fp);
    }
    else
    {
        LOG_ERROR("Failed to create testfile.txt");
    }

    EventLoop loop;
    // std::cout << "main::baseloop_: " << loop.getId() << std::endl;
    InetAddress addr(8000, "192.168.0.102");
    // std::cout << "main::InetAddress: " << addr.toIpPort() << std::endl;
    EchoServer server(&loop, addr, "EchoServer");
    server.start();
    loop.loop();   // 启动mainLoop的底层Poller
    return 0;
}
