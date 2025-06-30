#pragma once

#include<unordered_map>
#include"noncopyable.h"
#include<vector>
#include"Timestamp.h"
//muduo库中多路事件分发的核心IO复用模块
class Channel;
class EventLoop;
class Poller
{
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop * loop);
    virtual ~Poller();

    // 给所有IO复用保留统一的接口  执行事件监听
    // 阻塞等待事件发生  返回事件发生的时间戳  通过 activeChannels 输出就绪的 Channel 列表
    virtual Timestamp poll(int timeoutMs, ChannelList * activeChannels) = 0;

    // 更新所感兴趣的事件
    //必须在Loop 线程中调用  同步更新 channels_ 映射和内核事件表
    virtual void updateChannel(Channel * channle) = 0;

    //删除在loop循环中管理的该Channel
    virtual void removeChannel(Channel * channle) = 0;

    // ChannelList中是否含有Channel，即参数Channel是否在当前Poller当中
    virtual bool hasChannel(Channel * channel) const;
    
    // Eventloop可以通过该接口获取默认的IO复用的具体实现
    static Poller * newDefaultPoller(EventLoop * loop);

    void assertInLoopThread() const
    {
        // add code...
        //owenrLoop_->assertInLoopThread();
    }
protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;//维护 fd → Channel 的映射关系，这是所有 Poller 实现的共同需求
private:
    EventLoop * ownerLoop_; //poller所属的事件循环Eventloop
};