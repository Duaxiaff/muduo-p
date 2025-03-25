#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;  //初始化当前线程的t_cachedTid
    void cacheTid()
    {
        if(t_cachedTid==0)
        {
            t_cachedTid=static_cast<pid_t>(::syscall(SYS_gettid));  //系统调用获得当前线程的tid
        }

    }
}