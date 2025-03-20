#pragma once
#include"Timestamp.h"
#include<vector>
#include<unordered_map>

class Channel;
class EventLoop;
//多路事件分发器的核心IO复用模块  Poller类
//Poller类是一个抽象基类，它定义了IO复用的接口，它有1个具体的实现类：EpollPoller  (muduo中还有一个PollPoller，用于支持poll函数)
//Poller类的主要作用是将Channel类中的关注的IO事件注册到Poller类中，然后Poller类将发生的IO事件通知Channel类
class Poller
{
public:
    using ChannelList = std::vector<Channel*>;  //ChannelList是一个Channel*类型的vector容器
    Poller(EventLoop* loop);
    virtual ~Poller() = default;  //基类的析构函数需要定义为虚函数，=default表示使用默认的析构函数
     
    //抽象函数，给所有IO复用保留统一接口
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;  //等待 IO 事件发生，并将活跃的通道记录到 activeChannels 中，并返回发生事件的时间
    virtual void updateChannel(Channel* channel) = 0;  //更新通道，添加或者更新通道(相当于就是把事件注册到Poller中)
    virtual void removeChannel(Channel* channel) = 0;  //移除通道

    //判断参数Channel是否在当前Poller中
    bool hasChannel(Channel* channel) const;

    //EventLoop对象通过这个接口可以获取默认的IO复用的具体实现，而不需要关心具体的实现细节
    static Poller* newDefaultPoller(EventLoop* loop);

protected:  //protected成员可以被派生类访问 而privated成员不能被派生类访问
    using ChannelMap =std::unordered_map<int,Channel*>;
    ChannelMap channels_;  //ChannelMap是一个int到Channel*的无序map容器  (相当于记录poller里面的Channel)

private:
    EventLoop* ownerLoop_;  //Poller类所属的EventLoop对象
};

