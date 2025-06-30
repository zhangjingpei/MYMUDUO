#pragma once

#include "Poller.h"
#include <sys/epoll.h>
#include <vector>
// #include<iostream>
// class Poller;

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop* loop);

    ~EpollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channle) override;
    void removeChannel(Channel* channle) override;


private:
    using EventList = std::vector<struct epoll_event>;
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    // 更新Channel通道
    void update(int operation, Channel* channel);

    int epollfd_;
    EventList events_;   // 由epoll_wait向这里面返回具体发生的事件，然后再返回Channel
};
