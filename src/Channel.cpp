#include <sys/epoll.h>
#include"Channel.h"


//定义静态常量成员变量，静态成员变量只能在类外定义
const int Channel::kNoneEvent=0;  //空事件
const int Channel::KReadEvent=EPOLLIN | EPOLLPRI;  //读事件  普通数据可读以及紧急数据可读
const int Channel::KWriteEvent=EPOLLOUT; //写事件

Channel::Channel(EventLoop *loop,int fd)
:loop_(loop),
fd_(fd),
events_(0),
revents_(0)
{
}

Channel::~Channel()
{

}
void Channel::update()  //注册fd当前的events
{

}
void Channel::handleEvent(Timestamp receiveTime)  //处理具体的事件
{

}