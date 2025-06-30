/*
获取当前线程的tid值
*/
#pragma once

namespace CurrentThread
{
    extern __thread int t_cachedTid;
    
    void cachedTid();

    inline int tid()
    {
        /*
        高频调用优化：tid() 在日志等场景频繁调用
        热路径优化：t_cachedTid != 0 是正常执行路径
        冷路径隔离：cacheTid() 是初始化时的罕见路径
        */
        if(__builtin_expect(t_cachedTid==0,0))
        {
            cachedTid();
        }
        return t_cachedTid;
    }
}


