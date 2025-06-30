#include"Poller.h"
#include"Channel.h"
Poller::Poller(EventLoop * loop)
:ownerLoop_(loop)
{

}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel * channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;

}

//为什么不将Poller * newDefaultPoller(EventLoop * loop);写在poller.cc里面
/*
这个函数是要返回具体的IO复用的对象指针
如果真的要写这个函数的话
必须要包含"PollPoller.h" "EpollPoller.h"
才能返回一个具体的实现，基类不能引用派生类，这会导致循环依赖：
基类 Poller 的实现依赖了派生类（违反依赖倒置原则）。
*/