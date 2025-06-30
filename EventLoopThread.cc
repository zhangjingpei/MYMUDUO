#include"EventLoopThread.h"
#include"EventLoop.h"
#include"logging.h"


EventLoopThread::EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string())
:loop_(nullptr)
,exiting_(false)
,thread_(std::bind(&EventLoopThread::threadFunc,this))
,mutex_()
,cond_()
,callback_(cb)
{
    LOG_INFO("EventLoopThread::EventLoopThread created. ");
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();  //为什么在loop_->quit()后还需要join呢
    }
}
EventLoop * EventLoopThread::startLoop()
{
    thread_.start(); //启动底层线程Thread类对象thread_中通过start中创建的线程
    EventLoop * loop = nullptr;  //为什么需要一个新loop指针？
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock,[this](){return loop_ != nullptr;});  //如何使用条件变量和互斥量
        loop = loop_;
    }
    return loop;
}


//下面的这个方法，是在单独的新线程中运行的
void EventLoopThread::threadFunc() 
{
    EventLoop loop; //创建一个单独的EventLoop对象 和上面的线程是一一对应的
    if(callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

