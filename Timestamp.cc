#include "Timestamp.h"
#include <iostream>
#include <time.h>

// 构造函数，初始化微秒数
Timestamp::Timestamp()
    : microSecondsSinceEpoch_(0)
{}
// 构造函数，用于初始化Timestamp对象
Timestamp::Timestamp(int64_t mircroSecondsSinceEpoch)
    : microSecondsSinceEpoch_(mircroSecondsSinceEpoch)
{}
Timestamp Timestamp::now()
{
    time_t ts = time(NULL);   // ts是long int型
    return Timestamp(ts);
}

// 将时间戳转换为字符串
std::string Timestamp::toString() const
{
    // 将时间戳转换为本地时间
    struct tm* tm = localtime(&microSecondsSinceEpoch_);
    // 定义一个字符数组，用于存储时间字符串
    char time_str[100];
    // 将本地时间格式化为字符串
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm);
    // 返回时间字符串
    return time_str;
}

// int main()
// {
//     Timestamp tm =  Timestamp::now();

//     std::cout<<tm.toString()<<std::endl;
// }