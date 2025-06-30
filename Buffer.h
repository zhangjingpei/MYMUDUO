#pragma once

#include <stddef.h>   //size_t
#include <string>
#include <vector>
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;   // 初始预留的prependabel空间大小
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initalSize = kInitialSize)
    : buffer_(kCheapPrepend + initalSize)
    , readIndex_(kCheapPrepend)
    , writeIndex_(kCheapPrepend)
    {}
    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }
    size_t prependableBytes() const { return readIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const { return begin() + readIndex_; }

    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            // 说明从缓冲区中已经读取了len个数据，但还是有一些其他可读的数据在缓冲区
            readIndex_ += len;
        }
        else   // 否则，则读完全部的可读数据
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readIndex_ = kCheapPrepend;
        writeIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的BUffer数据，转换成string的数据返回
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);   // 上一句把缓冲区的数据已经读出来，这里肯定要对缓冲区进行回复
        return result;
    }

    // 根据len判断剩余空间是否还够写，不够写则扩容
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    char* beginWrite()  // 不能返回const  因为只能返回const char* 或const T&
    {
        return begin() + writeIndex_;   // 获取写指针的位置
    }

    // 把[data,data+len]内存上的数据添加到writable缓冲区中
    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writeIndex_ += len;
    }

    const char* beginWrite() const { return begin() + writeIndex_; }

    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);

    // 通过fd发送数据
    ssize_t sendFd(int fd, int* saveErrno);

private:
    char* begin() { return &(*buffer_.begin()); }
    const char* begin() const
    {
        return &(*buffer_.begin());
    }   // C++ 不允许仅仅因为返回类型不同而重载函数。

    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writeIndex_ + len);
        }
        else   // 将可读区域的起始位置移到kCheapPrepend从而空出更多的写空间
        {
            size_t readable = readableBytes();   // 可读的字节数
            std::copy(begin() + readIndex_, begin() + writeIndex_, begin() + kCheapPrepend);
            readIndex_ = kCheapPrepend;
            writeIndex_ = readIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};