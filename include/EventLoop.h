#pragma once
#include "noncopyable.h"
#include <functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

#include"CurrentThread.h"
#include"Timestamp.h"



class Channel;
class Poller;

//事件循环类  相当于reactor模式中的reactor类
class EventLoop:noncopyable
{
public:
    using Functor = std::function<void()>; //定义一个回调的类型
    EventLoop();
    ~EventLoop();
    
    //开启事件循环
    void  loop();
    //退出事件循环
    void quit();

    Timestamp pollReturnTime()const {return pollRetureTime_;}

    //在当前loop中执行  注册回调函数
    void runInLoop(Functor cb);
    //把上层注册的回调函数cb放入队列中 唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);

    //通过eventfd唤醒loop所在的线程（mainreactor 用来唤醒subreactor）
    void wakeup();

    //下面这三个相当于都是eventloop提供一个接口去调用poller里面的函数
    void updateChannel(Channel *channel);  //调用poller的相应方法，注册fd的events事件
    void removeChannel(Channel *channel);  //删除现在的channel
    bool hasChannel(Channel *channel);   //判断channel是否在poller中
    //判断一下eventloop这个对象是否是在线程里面
    bool isInLoopThread() const{return threadId_== CurrentThread::tid();}  //threadId_是创建时候的线程id CurrentThread::tid()是当前线程的id

private:
    void handleRead();   //eventfd返回的文件描述符wakeupFd_绑定的事件回调 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait
    void doPendingFunctors(); //执行上层回调
    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;    //表示是否是在循环，原子操作
    std::atomic_bool quit_;   //标识是否退出loop循环
    
    const pid_t threadId_;  // 记录当前EventLoop是被哪个线程id创建的 即标识了当前EventLoop的所属线程id
                            //pid_t是一个数据类型，用来表示线程idor进程id

    Timestamp pollRetureTime_;  //Poller返回发生事件的Channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;  //作用：当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 通过该成员唤醒subLoop处理Channel 
    std::unique_ptr<Channel> wakeupChannel_;

    std::atomic_bool callingPendingFunctors_;  //标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;    // 存储loop需要执行的所有回调操作
    std::mutex mutex_;   //需要互斥锁 来保护上面vector容器的线程安全操作
    ChannelList activeChannels_; //Poller检测到当前有事件发生的所有Channel列表
};