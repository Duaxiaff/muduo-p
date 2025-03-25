#include "Poller.h"
#include "EPollPoller.h"

#include<stdlib.h>

//这个函数的目的是根据环境变量来决定创建并返回哪种类型的轮询器（Poller）实例
Poller* Poller:: newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;
    }
    else
    {
        return new EPollPoller(loop);  //生成一个epoll实例
    }
}