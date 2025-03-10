#pragma once
#include "Timestamp.h"

class EventLoop;
class Channel
{
public:
    Channel(EventLoop *loop,int fd);//构造函数
    ~Channel();

    //fd上有事件发生的时候，poller通知有事件发生，然后在handleEvent在EventLoop::loop()中调用处理事件  
    //输入参数用一个时间戳的类，输入接收的时间
    void handleEvent(Timestamp receiveTime);

    //定义回调函数，设置回调函数对象 一个文件描述符会发生可读、可写、关闭、错误等事件，当发生相应的事件就可以直接调用相应的处理函数
    void setReadCallback()  //可读
    {

    }
    void setWriteCallback() //可写
    {

    }   
    void setCloseCallback() //关闭
    {

    }
    void setErrorCallback() //错误
    {

    }



private:
    int fd_;   //fd,  poller监听的对象
    int events_;  //注册fd感兴趣的事件
    int revents_;  //poller返回的具体发生事件
};
