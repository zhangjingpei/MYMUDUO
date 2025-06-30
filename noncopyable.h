#pragma once

/*
noncopyable被继承之后，派生类对象可以正常的析构和构造，但是派生类
对象无法进行拷贝赋值
*/
class noncopyable
{
protected:
    noncopyable()=default;
    ~noncopyable()=default;
public:
    noncopyable(const noncopyable&)=delete;
    noncopyable&operator=(const noncopyable&)=delete;
};