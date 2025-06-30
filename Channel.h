#pragma once
#include "Timestamp.h"
#include "noncopyable.h"
#include <functional>
#include <memory>
/*
Channel可以理解为通道，封装了sockefd和其感兴趣的event，如EPOLLIN、EPOLLOUT
Channel的作用是负责将文件描述符注册到事件路由分发器(epoll)中，
还绑定了poller返回的具体实现---事件处理器
为什么还需要Eventloop？每个 Channel 必须明确归属于某个特定的 EventLoop（即I/O线程）。


*/
class EventLoop;
class Timestamp;
class Channel : noncopyable
{
public:
    // 事件回调函数  std::function相当于函数指针
    typedef std::function<void()> EventCallback;   // using EventCallback = std::function<void()> ;

    // 只读事件回调函数
    /*
    当有数据可读（或对端关闭连接等）时调用的回调函数。
    通常传递一个时间戳（Timestamp）参数，表示事件发生的时间
    */
    typedef std::function<void(Timestamp)> ReadEventCallback;

private:
    // 事件循环指针 -- channel所属的事件循环
    EventLoop* loop_;   // 确保所有操作在正确的 I/O 线程执行

    // 文件描述符
    int fd_;   // 析构时不关闭 fd（由上层对象管理）

    // 事件
    /*
    //Channel 可以同时注册多个事件类型（如既监听可读，又监听可写），
    //因此 events_ 是一个 事件集合 （bitmask），而非单一事件。
    */
    int events_;   // 该Channel当前注册到Poller（如epoll）中感兴趣的事件


    // 实际发生的事件
    int revents_;   // 由Poller返回，然后Channel根据这些事件调用相应的回调。

    /*
    索引
    更高效的知道该channel在poller中的状态
    比如kNew表示还未注册到poller中，
    kAdded已经注册到poller中，
    kDeleted表示已经从poller中删除
    */
    int index_;

    // 因为channel通道里面能够通过poller获知fd最终发生的具体的revents_，所以channel类负责具体事件的回调操作
    ReadEventCallback readCallback_;   // 分离读和其他事件 读需要额外的时间戳信息
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

    /*
    可以利用.lock来提升为强智能指针，若提升成功访问，提升失败，说明这个所观察的资源已经被释放了
    */
    std::weak_ptr<void> tie_;   // 在多线程里面利用weak_ptr监听所观察的这个资源的生存状态
    bool tied_;

    static const int kNoneEvent;    // 无事件
    static const int kReadEvent;    // 读事件标志 (EPOLLIN | EPOLLPRI)
    static const int kWriteEvent;   // 写事件标志 (EPOLLOUT)


    void update();   // 更新Poller中的事件注册

    void handleEventWithGuard(Timestamp receiveTime);   // 带生命周期保护的事件处理

public:
    // 构造函数
    Channel(EventLoop* loop, int fd);

    // 析构函数
    ~Channel();


    // fd得到poller通知以后，处理事件的函数，根据实际发生的事件，调用相应的回调函数
    void handleEvent(Timestamp receiveTime);   // 事件处理入口

    // channel并不知道具体去做什么样的回调，它只是被动的去执行给它设置的回调
    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb)
    {
        readCallback_ = std::move(cb);
    }   // move只是将函数指针销毁了，资源转给readCallback_
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revents) { revents_ = revents; }   // poller设置实际发生的事件
    bool isNoneEvent() const
    {
        return events_ == kNoneEvent;
    }   // 当前的channel是否已经注册了感兴趣的事件
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // 设置fd相应的事件状态
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }   // update就是在调用epoll_ctl注册事件
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    int index() const { return index_; }
    void set_index(int index) { index_ = index; }

    EventLoop* ownerLoop() { return loop_; }

    void remove();   // 向poller删除该fd
};
