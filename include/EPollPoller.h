#pragma once
#include"Poller.h"
#include<sys/epoll.h>
#include "Timestamp.h"

class EPollPoller:public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    //重写Poller类的纯虚函数
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    //把活跃的事件放到活跃事件的 vector 中
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    // 更新channel通道 其实就是调用epoll_ctl
    void update(int operation, Channel *channel);
    using EventList = std::vector<epoll_event>;  //epoll_event的vector

    int epollfd_; //epoll_create返回的文件描述符
    EventList events_;  //定义一个数组，用于存放epoll_event

    static const int kInitEventListSize = 16;  //初始化events_的大小
};