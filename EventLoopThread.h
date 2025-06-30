/*
为每一个线程创建一个EventLoop对象，
*/


#pragma once

#include"noncopyable.h"
#include"Thread.h"
#include<mutex>
#include<iostream>
#include<condition_variable>
#include<string>

class EventLoop;
class EventLoopThread:noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThread(const ThreadInitCallback &cb,
                    const std::string &name);
    ~EventLoopThread();
    EventLoop * startLoop();


private:
    void threadFunc();
    EventLoop * loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;

};