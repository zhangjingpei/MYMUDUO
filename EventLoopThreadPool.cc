#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"


EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg)
    : baseLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
// threads_默认为空，loop_也是
{}


EventLoopThreadPool::~EventLoopThreadPool() {}
void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    started_ = true;
    for (int i = 0; i < numThreads_; i++) {
        char buf[name_.size() + 32];   // 为什么+32
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(
            t->startLoop());   // 底层创建线程，绑定一个新的EventLoop，并返回该loop的地址
    }

    if (numThreads_ == 0 && cb)   // 整个服务端只有一个线程运行baseLoop
    {
        cb(baseLoop_);   // cb只有在此时执行
    }
}

// 如果工作在多线程下，baseloop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
EventLoop* EventLoopThreadPool::getNextLoop()
{
    // 如果只设置一个县城，也就是只有一个mainReactor
    // 那么轮询只有一个线程 getNextLoop()返回baseLoop_;
    EventLoop* loop = baseLoop_;

    // 通过轮询获取下一次处理事件的loop
    // 如果没设置多线程数量，则不会进去，相当于直接返回baseLoop_
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        // 轮询
        if (next_ >= loops_.size()) next_ = 0;
    }
    return loop;
}

// 获取所有的EventLoop
std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if (!loops_.empty()) {
        return std::vector<EventLoop*>(1, baseLoop_);
    }
    else
        return loops_;
}
