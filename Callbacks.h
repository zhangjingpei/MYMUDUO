#pragma once

#include<memory>
#include<functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

//水位线 接收方和发送放速率应该接近
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&,size_t)>;

using MessageCallback = std::function<void(const TcpConnectionPtr&,Buffer *,Timestamp)>;