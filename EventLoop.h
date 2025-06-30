/*
EventLoop相当于Reactor反应堆
Poller相当于Demultiplex 事件分发器

*/

#pragma
#include "CurrentThread.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include <vector>

class Channel;
class Poller;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();
    void loop(); //开启事件循环

    void quit(); //退出事件循环

    Timestamp pollReturnTime() const { return pollReturnTime_; } //获取poller的返回时间

    void runInLoop(Functor cb); //在当前Loop执行cb

    void queueInLoop(Functor cb); //把cb放入队列中，唤醒loop所在的线程，执行cb

    void wakeup(); //唤醒loop所在的线程的

    //Eventloop的方法 => Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    void hasChannel(Channel *channel);

    //判断Eventloop是否在自己的线程里面
    void assertInLoopThread()
    {
        if (!isInLoopThread())
            abortNotInLoopThread();
    }

    bool isInLoopThread() const
    {
        return threadId_ == CurrentThread::tid();
    } //如果当前eventLoop没有在当前线程中，则执行queueInLoop

    //bool eventHandling() const {return eventHandling_;}

private:
    using channelList = std::vector<Channel *>;
    std::atomic_bool looping_; // 原子操作，通过CAS实现
    std::atomic_bool quit_;    // 在其他线程中调用EventLoop的quit，标志：退出循环
    //std::atomic_bool eventHandling_;//

    const pid_t threadId_; // 记录当前loop的所在的线程ID；

    Timestamp pollReturnTime_; //记录poller返回发生事件的Channels的时间点

    std::unique_ptr<Poller> poller_;

    //主要作用：当mainLoop获取一个新用户的Channel，通过轮询算法选择一个subLoop，
    //通过wakeupFd_通知/唤醒subLoop处理Channle
    // 可以通过poller来注册、等待
    int wakeupFd_;                           //现有wakeupFd_才能创建wakeupChannel_
    std::unique_ptr<Channel> wakeupChannel_; //wakeupChannle将wakeupfd封装起来

    channelList activeChannels_;
    Channel *currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_; //标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendignFunctors_;    // 存放loop需要执行的回调操作
    //互斥锁用来保护上面容器的线程安全操作
    std::mutex mutex_;

    void abortNotInLoopThread();
    void handleRead();        // waked up
    void doPendingFunctors(); //执行上层回调

    void printActiveChannels() const;
};