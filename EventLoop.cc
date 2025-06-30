#include "EventLoop.h"

#include <sys/eventfd.h>   //eventfd

#include <cassert>   // assert

#include "Channel.h"
#include "Poller.h"
#include "errno.h"
#include "logging.h"
#include "unistd.h"   //read

// 线程局部变量，确保一个线程只能创建一个EventLoop
__thread EventLoop* t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间。
const int kPollTimeMs = 10000;

/*
创建wakeupFd_,用来notify唤醒subReactor处理新来的channel
*/
int createEventfd()
{
    int evfd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (evfd < 0)
    {
        LOG_FATAL("eventfd error:%d", errno);
    }
    return evfd;
}

EventLoop::EventLoop()
: looping_(false)
, quit_(false)
, callingPendingFunctors_(false)   // 刚开始没有需要调用的函数
, threadId_(CurrentThread::tid())
, poller_(Poller::newDefaultPoller(this))
, wakeupFd_(createEventfd())
, wakeupChannel_(new Channel(this, wakeupFd_))   // mainReactor通过wakeupFd_去唤醒subReactor
, currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)   // 如果在这个线程中已经创建了EventLoop
                              // 就不要在创建了
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d", t_loopInThisThread, threadId_);
    }
    else   // 否则 t_loopInThisThread指向这个EventLoop
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupFd_的事件类型以及发生事件后的回调操作
    // 确保在监听事件前，回调已就绪。即使事件立即到达，也能正确调用 handleRead。
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();   // 每一个EventLoop都将监听wakeupChannel_的EPOLL读事件了
}   // end of EventLoop()

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();   // 设置感兴趣事件为无
    wakeupChannel_->remove();       // 将wakeupChannel从poller中删除
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one,
                       sizeof(one));   // 清空缓冲区，保证epoll_wait不循环处理
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

void EventLoop::loop()
{
    assert(!looping_);      // 断言是否在循环中，保证只启动一次循环
    assertInLoopThread();   // 判断该事件循环是否处于当前线程
    looping_ = true;
    quit_ = false;   // 如果先调用的quit()在调用的loop() 需要将quit_ 置为false
    LOG_INFO("EventLoop:%p threadId:%d  start looping.", this, threadId_);
    while (!quit_)   // quit_为false的时候开启循环
    {
        activeChannels_.clear();   // 清空活跃的channels //为什么？

        // 监听两类fd，一种是clientfd，一种是wakeupfd-->mainLoop与subLoop之间的通信
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        // 如果有事件发生，poll会将发生的事件注册到activeChannels列表

        for (Channel* channel : activeChannels_)
        {
            // poller监听那些Channel发生事件了，然后上报给EventLoop，通知channle处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // printActiveChannels();

        // 执行当前EventLoop事件循环所需要处理的回调操作
        /*
        IO线程也就是mainLoop accept:接收新用户的连接返回fd <--Channel
        mainLoop负责新用户的连接
        因此新连接需要分配给subLoop
        mainLoop事先注册一个回调cb <-- 需要subLoop来执行
        subLoop现在正在阻塞，mainLoop通过wakeup来唤醒subLoop执行之前mainLoop注册的cb操作
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

// 退出事件循环
// 1、loop在自己的线程中调用这个quit成功了，说明当前线程已经执行完毕了loop()函数的poller_->poll并退出
// 2、如果不是当前EventLoop所属线程中调用quit退出EventLoop，需要唤醒EventLoop所属现成的epoll_Wait
void EventLoop::quit()
{
    quit_ = true;

    // 如果是在其他线程中调用了quit，比如在一个subLoop中，调用了mainLoop中的quit,需要先唤醒mainLoop中的阻塞部分
    if (!isInLoopThread()) wakeup();
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())   // 是否在当前线程中
    {
        cb();   // 在的话执行回调
    }
    else   // 在非当前线程中执行cb，就需要唤醒另外的EventLoop所在的线程执行cb
    {
        queueInLoop(cb);
    }
}

//
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendignFunctors_.emplace_back(cb);
    }

    /*
    callingPendingFunctors_的意思是，当前loop正在执行回调中，但是loop的pendingFuctors_中又加入了新的回调
    需要通过wakeup写事件来唤醒相应的需要执行上面回调操作的loop所在的线程
    让loop()下一次poller_->poll()不在阻塞(阻塞的话会延迟前一次新加入的回调的执行)
    继续执行pendingFunctors_中的回调函数
    */
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();   // 唤醒loop所在的线程
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8.\n", n);
    }
}

// 通过EventLoop的方法调用Poller的方法
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}
void EventLoop::hasChannel(Channel* channel)
{
    poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL("EventLoop::abortNotInLoopThread - EventLoop %p, was created in "
              "threadId %d, current Thread id = %d",
              this,
              threadId_,
              CurrentThread::tid());
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        // 加锁：避免处理过程中又有新的回调被投递
        std::unique_lock<std::mutex> lock(mutex_);
        // 交换容器，减少锁持有时间（避免阻塞其他线程投递任务）
        functors.swap(pendignFunctors_);

        // 交换的方式减少了锁的临界区范围。
        // 同时避免了死锁，如果执行functor()在临界区内，
        // 且functor()调用了queueInLoop()就会产生死锁
        // 为什么
    }

    for (const Functor& functor : functors)
    {
        functor();
    }
    callingPendingFunctors_ = false;
}
