#pragma once
#include "Timestamp.h"
#include <functional>
#include <memory>
class EventLoop;
class Channel
{
public:
    using ReadEventCallback=std::function<void(Timestamp)>; //使用std::function<>来定义回调类型
    using EventCallback = std::function<void()>;

    Channel(EventLoop *loop,int fd);//构造函数
    ~Channel();

    //fd上有事件发生的时候，poller通知有事件发生，然后在handleEvent在EventLoop::loop()中调用处理事件  
    //输入参数用一个时间戳的类，输入接收的时间  实际调用下面的回调函数（触发回调）
    void handleEvent(Timestamp receiveTime);

    //注册回调函数 一个文件描述符会发生可读、可写、关闭、错误等事件（负责的是具体事件），当发生相应的事件就可以直接调用相应的处理函数
    void setReadCallback(ReadEventCallback cb)  //可读   当Poller（轮询器）检测到fd上有可读事件时，handleEvent方法会调用这个回调函数(存疑)
    {
        readCallback_=std::move(cb);  //因为这里cb只是个临时对象，之后不再使用，直接用move可以避免复制，优化性能
    }
    void setWriteCallback(EventCallback cb) //可写
    {
        writeCallback_=std::move(cb);
    }   
    void setCloseCallback(EventCallback cb) //关闭
    {
        closeCallback_=std::move(cb);
    }
    void setErrorCallback(EventCallback cb) //错误
    {
        errorCallback_=std::move(cb);
    }

    //设置fd感兴趣的事件，注册到事件监听器上或者从事件监听器上移除 相当于epoll_ctl add delete
    void enableReading()  //启动读取事件
    {
        events_|=KReadEvent;
        update();  //注册到事件监听器上
    }
    void disableReading()  //禁用读取事件
    {
        events_&=~KReadEvent;
        update();
    }
    void enableWriting()  //启动写入事件
    {
        events_|=KWriteEvent;
        update();
    }
    void disableWriting() //禁用写入事件
    {
        events_&=~KWriteEvent;
        update();
    }
    void disableAll() //禁用所有事件
    {
        events_=kNoneEvent;
        update();
    }

    //返回一下当前fd的事件状态（bool）声明为常量成员函数，表示只读，表明不会改变任何的成员变量
    bool isNoneEvent() const
    {
        return events_ == kNoneEvent;
    }
    bool isWriting() const
    {
        return events_ & KWriteEvent;
    }
    bool isReading() const
    {
        return events_ & KReadEvent;
    }


    void remove(); //把当前的Channel删掉

    //防止当channel被手动remove掉 channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);


private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    //静态常量成员变量  被初始化后，初始化的值在整个程序运行期间都不会变，并且所有Channel的实例都会共享这个值
    static const int kNoneEvent;
    static const int KReadEvent;
    static const int KWriteEvent;

    EventLoop* loop_; //事件循环
    const int fd_;   //fd,  poller监听的对象
    int events_;  //注册fd感兴趣的事件
    int revents_;  //poller返回的具体发生事件 （fd实际发生的事件）

    std::weak_ptr<void> tie_;  //不会增加引用的次数，只是提供一个观察对象的方式
    bool tied_;

    //可以获知fd最终发生的具体事件event，下面的变量负责具体事件的回调存储。
    ReadEventCallback  readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
