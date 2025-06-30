#pragma once
#include"noncopyable.h"
#include<functional>
#include<iostream>
#include<thread>
#include<memory>
#include<unistd.h>
#include<atomic>
#include<string>
#include"CurrentThread.h"
class Thread:noncopyable
{
public:
    using ThreadFuc = std::function<void()>;

    explicit Thread(ThreadFuc func,const std::string &name=std::string());
    ~Thread();
    
    void start();
    void join();

    bool started() {return started_;}
    pid_t tid() const {return tid_;}

    const std::string& name() {return name_;}
    static int numCreated() {return numCreated_;}

private:
    void setDefalutName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_; //为什么不这么写：std::thread thread_;
    pid_t tid_;
    ThreadFuc fuc_;//存储线程函数的变量
    std::string name_;
    static std::atomic<int> numCreated_;  //为什么用原子变量  多个线程同时操作

};