#pragma once

#include"Poller.h"

class PollPoller:public Poller
{
private:
    Timestamp poll(int timeoutMs, ChannelList * activeChannels);
    void updateChannel(Channel * channle);
    void removeChannel(Channel * channle);
    bool hasChannel(Channel * channel) const;
public:
    PollPoller(EventLoop * loop);

    ~PollPoller();
};


