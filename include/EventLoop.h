#pragma once


//事件循环类
class EventLoop
{
public:
    EventLoop();
    ~EventLoop();


    void updateChannel(Channel *channel);  //调用poller的相应方法，注册fd的events事件
    void removeChannel(Channel *channel);  //删除现在的channel

};