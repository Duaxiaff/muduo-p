#pragma once
#include "noncopyable.h"
#include <functional>
#include <condition_variable>
#include "Thread.h"
#include <mutex>

class EventLoop;
class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback= std::function<void(EventLoop *)>;  //线程初始化回调函数
    EventLoopThread(const ThreadInitCallback &cb=ThreadInitCallback(),const std::string &name=std::string());  //构造函数
    ~EventLoopThread();  //析构函数
    EventLoop *startLoop();  //开启循环
private:
   void threadFunc();  //线程函数
   EventLoop *loop_;  //EventLoop对象指针
   bool exiting_; //是否退出
   Thread thread_;  //线程对象
   std::mutex mutex_;  //互斥锁
   std::condition_variable cond_; // 条件变量
   ThreadInitCallback callback_;  //线程初始化回调函数
};