#include "CurrentThread.h"
#include <sys/syscall.h>
#include <unistd.h> //pid_ts
namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cachedTid()
    {
        if (t_cachedTid == 0)
        {
            // 通过linux系统调用获取当前线程的tid值
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}
