#include "EpollPoller.h"

#include "Poller.h"
#include <stdlib.h>
Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    
    return new EpollPoller(loop);
    
}
