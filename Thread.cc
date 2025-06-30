#include"Thread.h"
#include"semaphore.h"
#include"logging.h"


std::atomic<int> Thread::numCreated_{0};


Thread::Thread(ThreadFuc func,const std::string &name)
:started_(false)
,joined_(false)
,tid_(0)
,fuc_(func)
,name_(name)
{
    setDefalutName();
}

Thread::~Thread()
{
    if(started_ && !joined_)  //为什么？原理是什么
    {
        thread_->detach();  
    }
}

void Thread::start() //一个Thread对象，记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false,0);
    //开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){  //为什么有引用？
        tid_ = CurrentThread::tid();
        LOG_INFO("Thread::start() create thread id:%d",tid_);
        sem_post(&sem);
        fuc_(); //开启一个线程，专门执行该线程函数
    }

    ));

    //这里必须等待获取上面新创建的现成的tid值  
    //因为线程是以不可预知的速度前进的，
    //万一在start结束之后，创建的线程还没有到获取tid的值的话，则会造成不可知的状态
    sem_wait(&sem);


}
void Thread::join()
{
    joined_ = true;
    thread_ ->std::thread::join();
}


void Thread::setDefalutName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32]={0};
        snprintf(buf,sizeof(buf),"Thread%d",num);
        name_ = buf;
    }
    
}