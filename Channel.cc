#include "Channel.h"
#include "EventLoop.h"
#include "logging.h"
#include <assert.h>
#include <cassert>
#include <poll.h>
#include <sstream>
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
    // if(loop_->isInLoopThread()) // 这个事件循环是否在所属线程中
    // {
    //     assert(loop_->hasChannel(this));//这个Channel(fd)是否在所属的Eventloop中
    // }
}
// channel的tie方法什么时候调用  // 一个TcpConnection被创建的时候
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/*
当改变Channel所表示的fd的时间后，update负责在poller里面更改fd相应的事件epoll_ctl
Channel不能直接调用另外一个poller类
但是EventLoop是由Channel和poller组成的
可以通过EventLoop来调用poller

*/
void Channel::update()
{
    // 通过Channel所属的Eventloop，调用poller的相应的方法，注册fd的events事件

    loop_->updateChannel(this);
}

void Channel::remove()
{
    // 通过Channel所属的Eventloop，调用poller的相应的方法，注册fd的events事件

    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的Channel发生的具体事件，然后Channel调用相应的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("=====\nChannel handleEvent revents:%d\n========\n", revents_);
    // 关闭
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
            closeCallback_();
    }

    // 错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
            errorCallback_();
    }

    // 可写
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
            writeCallback_();
    }
    // 可读
    if ((revents_ & EPOLLIN) || (revents_ & EPOLLPRI))
    {
        if (readCallback_)
            readCallback_(receiveTime);
    }
}