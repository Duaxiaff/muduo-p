#pragma once
#include "noncopyable.h"
#include <functional>
#include <string>
#include <memory>
#include <vector>
class EventLoop;
class EventLoopThread;

//用于管理一组事件循环线程
class EventLoopThreadPoll : noncopyable
{
public:
    using ThreadInitCallback= std::function<void(EventLoop *)>;  //线程初始化回调函数
    EventLoopThreadPoll(EventLoop *baseLoop,const std::string &nameArg);
    ~EventLoopThreadPoll();

    void setThreadNum(int numThreads){numThreads_=numThreads;}  //设置线程数

    void start(const ThreadInitCallback &cb=ThreadInitCallback());  //启动线程池

    // 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops(); //获取所有的EventLoop对象

    bool started() const {return started_;} //是否已经启动
    const std::string name() const {return name_;}  //获取线程池的名字


private:
    EventLoop *baseLoop_;  // 用户使用muduo创建的loop 如果线程数为1 那直接使用用户创建的loop 否则创建多EventLoop对象
    std::string name_;  //线程池的名字
    bool started_;  //是否已经启动
    int numThreads_;  //线程数
    int next_;  //新链接到来，所选择EventLoop的索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_;  //线程数组
    std::vector<EventLoop *>loops_;  //线程池中EventLoop的列表，指向的是EVentLoopThread线程函数创建的EventLoop对象

};