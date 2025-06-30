#pragma once

#include<functional>
#include"noncopyable.h"
#include<string>
#include<iostream>
#include<memory>
#include<vector>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThreadPool(EventLoop * baseLoop, const std::string&nameArg);

    ~EventLoopThreadPool();

    //设置线程池数量
    void setThreadNum(int numThreads){numThreads_ = numThreads;}

    void start(const ThreadInitCallback &cb=ThreadInitCallback());

    //如果工作在多线程下，baseloop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
    EventLoop * getNextLoop();

    //获取所有的EventLoop
    std::vector<EventLoop *> getAllLoops() ;

    bool started() const {return started_;}

    const std::string name() const {return name_;}


private:
    EventLoop * baseLoop_; //用户使用muduo创建的loop，如果线程数为1，那么直接使用用户创建的loop，否则创建多个EventLoop
    std::string name_;//线程池的名称，通常由用户指定，线程池中EventLoopThread依赖于线程池名称。
    
    bool started_;//线程池是否已经启动

    int numThreads_; //线程池中线程的数量；

    int next_; //新连接到来，所选择的EventLoop的索引

    std::vector<std::unique_ptr<EventLoopThread> > threads_;//IO线程中的EventLoopThread列表
    std::vector<EventLoop * > loops_; // 线程池中EventLoop的列表，指向的是EVentLoopThread线程函数创建的EventLoop对象。

};  