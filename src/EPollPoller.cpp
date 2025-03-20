#include <errno.h>
#include <unistd.h>
#include <string.h>
#include"EPollPoller.h"
#include"EventLoop.h"
#include"Logger.h"
#include"Channel.h"

//errno是一个全局变量，用于指示最近一次系统调用或库函数调用的错误代码
EPollPoller::EPollPoller(EventLoop* loop)
: Poller(loop),  //调用基类的构造函数
epollfd_(::epoll_create1(EPOLL_CLOEXEC)),  //创建一个epoll实例
events_(kInitEventListSize)  //初始化events_的大小
{
    if(epollfd_<0)
    {
        LOG_FATAL("epoll_create error:%d\n",errno);
    }
}
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);  //关闭epoll实例 使用::是为了调用全局的close函数，而不是其他作用域下的close函数
}

//等待 IO 事件发生，并将活跃的通道记录到 activeChannels 中，并返回发生事件的时间
Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_INFO("func=%s -> fd total conut=%lu\n",__FUNCTION__,channels_.size());//输出当前的函数名以及当前的channel的数量 _FUNCTION_是一个宏，表示当前的函数名
    int numEvents=::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);  //等待事件发生  timeoutMs等待事件发生的超时时间
    int saveErrno=errno;  //保存errno
    Timestamp now(Timestamp::now());  //定义一个Timestamp类的对象now，调用now函数获取当前时间并返回一个Timestamp对象，将其初始化对象now
    if(numEvents>0)//有事件发生
    {
        LOG_INFO("%d events happened\n",numEvents);
        fillActiveChannels(numEvents,activeChannels);  //把活跃的事件放到活跃事件的 vector 中
        if(numEvents==events_.size())//如果事件数放不下，扩容
        {
            events_.resize(events_.size()*2);
        }
    }
    else if(numEvents==0)  //在指定的超时时间内没有事件发生
    {
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }
    else{
        if(saveErrno!=EINTR)  //如果不是被信号中断 EINTR表示被信号中断
        {
            errno=saveErrno;
            LOG_ERROR("EPollPoller::poll() error!\n");
        }
    }
    
}
void EPollPoller::updateChannel(Channel* channel)
{

}
void EPollPoller::removeChannel(Channel* channel)
{

}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for(int i=0;i<numEvents;i++)
    {
        Channel* channel= static_cast<Channel*>(events_[i].data.ptr);  //获取发生事件的channel
        channel->set_revents(events_[i].events);  //设置发生的具体事件
        activeChannels->push_back(channel);  //将发生事件的channel放到活跃事件的 vector 中
    }
}