#pragma once

//
#include <cstdint>
#include<string>
#include<iostream>
class Timestamp
{
public:
    Timestamp(); // 构造函数
    explicit Timestamp(int64_t mircroSecondsSinceEpoch); // 带参数的构造函数
    std::string toString() const; // 将时间戳转换为字符串
    bool valid() const; // 判断时间戳是否有效
    static Timestamp now(); // 获取当前时间戳

    // 时间戳类
private:
    int64_t microSecondsSinceEpoch_; //有符号的长整数，表示自1970年1月1日以来的微秒数
};