#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>   //write

// 从fd上读取数据
/*
Poller工作在水平模式，Buffer缓冲区是有大小的，但是从fd上读取数据的时候，不知道数据的最终大小
@description: 从socket读到缓冲区的方式是使用readv先读至buffer_
Buffer_空间如果不够绘读入到栈上65536个字节大小的空间，然后以append的方式追加到buffer_
既考虑了避免系统调用带来开销，又不影响数据的接接收
*/
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    // 栈额外空间，用于从套接字往出读时，当buffer_暂时不够用时，
    // 待buffer_重新分配足够空间后，在把数据交换给buffer_。
    char extrabuffer[65536] = {0};

    /*
    struct iovec
    {
        void *iov_base;	/* Pointer to data.
        size_t iov_len;	/* Length ofdata.
    };
        iov_base指向的缓冲区是readv所接收的数据或是write要发送的数据
        iov_len在各种情况下分别确定了接受的最大长度或实际写入的长度
    */

    // 使用iovec分配两个连续的缓冲区
    struct iovec vec[2];

    const size_t writeable = writableBytes();   // 可写区域大小

    // 第一块缓冲区：指向写空间
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writeable;

    // 第二块缓冲区，指向栈空间
    vec[1].iov_base = extrabuffer;
    vec[1].iov_len = sizeof(extrabuffer);

    const int iovcnt = (writeable < sizeof(extrabuffer) ? 2 : 1);   // 一次最多读64K的数据
    // 解释一下readv
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n < writeable)
    {
        writeIndex_ += n;
    }
    else
    {
        writeIndex_ = buffer_.size();
        append(extrabuffer, n - writeable);
    }
    return n;
}

// 通过fd发送数据
ssize_t Buffer::sendFd(int fd, int* saveErrno)
{
    //::write 发送消息给对端
    // write() writes up to count bytes from the buffer starting at buf to the file referred to by
    // the file descriptor fd.
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}