#include"EpollPoller.h"
#include"EventLoop.h"
#include "Poller.h"       // 添加基类头文件
#include"logging.h"
#include"Channel.h"

#include <unistd.h>
#include<errno.h>
#include <cstring>


const int kNew = -1;  //某个Channel还没添加至Poller  对应Channel中的成员变量 index_
const int kAdded = 1;  //已经添加
const int kDeleted = 2; //已经删除Channel


class EventLoop;
EpollPoller::EpollPoller(EventLoop * loop)
:Poller(loop) // 基类构造函数
,epollfd_(::epoll_create1(EPOLL_CLOEXEC))  //生成epollfd_
,events_(kInitEventListSize) // vector<int>(kInitEventListSize)
{
    if(epollfd_ < 0)
        LOG_FATAL("epoll_create error:%d",errno);
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

void EpollPoller::update(int operation, Channel*channel)
{
    epoll_event event;
    ::memset(&event, 0, sizeof(event));
    event.data.fd=channel->fd();
    event.data.ptr = channel;
    event.events = channel->events();
    
    if((::epoll_ctl(epollfd_, operation, channel->fd(), &event))<0)
    {
        if(operation==EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n",errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
    
}



void EpollPoller::updateChannel(Channel * channle)
{
    Channel * channel = channle;
    const int index = channel->index(); //获取该channel的状态
    LOG_INFO("func=%s => fd=%d events=%d index=%d\n",__FUNCTION__,channel->fd(),channel->events(),index);
    if(index==kNew || index==kDeleted)
    {
        if(index==kNew)  // 如果没有添加到std::unordered_map<int, Channel*> channles_中，则添加
        {
            int fd=channel->fd();
            channels_[fd] = channel; //更新channels_
        }

        else // index==kDeleted
        {
            
        }
        channel->set_index(kAdded);//更新该channel的状态
        update(EPOLL_CTL_ADD,channel);//更新epoll所监听channel的事件
    }
    else // index = kAdded
    {
        int fd = channel->fd();
        if(channel->isNoneEvent()) //判断该channel是否对事件感兴趣
        {
            update(EPOLL_CTL_DEL, channel); //如果对任何事件不感兴趣，则删除该channel，即fd
            channel->set_index(kDeleted); //更新channel状态
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

void EpollPoller::removeChannel(Channel * channel)
{
    int fd = channel->fd();
    if(hasChannel(channel))
    {
        channels_.erase(fd);
        LOG_INFO("fuc=%s => fd=%d\n",__FUNCTION__, fd);
    }
    int index = channel->index();
    if(index==kAdded)
        update(EPOLL_CTL_DEL,channel);
    channel->set_index(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents,ChannelList * activeChannels) const
{
    for(int i=0;i<numEvents;i++)
    {
        Channel * channel = static_cast<Channel*>(events_[i].data.ptr); //channel是存放在发生事件列表中的每个事件的所属的真正的Channel
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

Timestamp EpollPoller::poll(int timeoutMs, Poller::ChannelList *activeChannels)
{
    LOG_INFO("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());
    int numEents = ::epoll_wait(epollfd_,&*events_.begin(),
                        static_cast<int>(events_.size()),timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEents>0)
    {
        LOG_INFO("%d events happend\n",numEents);
        fillActiveChannels(numEents,activeChannels);
        if(numEents == events_.size())
            events_.resize(events_.size()*2);
    }
    else if(numEents==0)
    {
        LOG_DEBUG("%s timeout!\n,__FUNCTION__");
    }
    else
    {
        if(saveErrno!=EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EpollPoller::poll() error");
        }
    }
    return now;

}
