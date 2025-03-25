#pragma once
#include <unistd.h>
#include <sys/syscall.h>

//提供一个方便的方式来获取和缓存当前线程的tid
//用命名空间来定义，因为所有的方法并不需要依赖任何特定的对象状态
namespace CurrentThread
{
    /*
       extern 关键字用于声明一个变量或函数在其他地方定义。它告诉编译器这个变量或函数的定义在当前文件之外，
    可能在另一个源文件中或者在一个库中。使用 extern 可以在不同的源文件中共享变量。
        它用于声明一个变量的实例对于每个线程都是唯一的。这意味着每个线程都有自己的 t_cachedTid 变量副本，线程之间的这个变量是相互独立的。
    */
    extern __thread int t_cachedTid;

    //用于获取当前线程的 TID 并将其缓存到 t_cachedTid 变量中
    void cacheTid();

    //内联函数，用来返回当前线程的tid
    inline int tid()
    {
        if(__builtin_expect(t_cachedTid==0,0))  //__builtin_expect 是一种底层优化
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}