#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,const std::string &name)  //构造函数
:loop_(nullptr)
,exiting_(false)
,thread_(std::bind(&EventLoopThread::threadFunc,this),name)
,mutex_()     //默认构造函数
,cond_()
,callback_(cb)
{

}

EventLoopThread::~EventLoopThread()  //析构函数
{
    exiting_=true;
    if(loop_!=nullptr)
    {
        loop_->quit();    //退出线程中绑定的事件循环
        thread_.join();  //等待线程结束
    }
}
EventLoop *EventLoopThread::startLoop()  //开启循环
{
    thread_.start();  //启动底层的新线程
    EventLoop *loop=nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_==nullptr)
        {
            cond_.wait(lock);   //等待条件变量
        }
        loop=loop_;
    }
    return loop;  //返回一个EventLoop对象指针
}

// 下面这个方法 是在单独的新线程里运行的
void EventLoopThread::threadFunc()  //线程函数
{
    EventLoop loop;  //创建一个独立的EventLoop对象  和上面的线程是一一对应的 也就是one loop per thread
    if(callback_)  //如果该线程有一些初始化的回调函数
    {
        callback_(& loop);  //调用回调函数
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_= &loop;  //将loop_指向新创建的EventLoop对象
        cond_.notify_one();
    }
    loop.loop();  //开启事件循环   执行EventLoop的loop() 开启了底层的Poller的poll()
    //会一直循环loop ，如果退出就执行下面的代码
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=nullptr;  //将loop_置为空

}