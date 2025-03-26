#pragma once
#include"noncopyable.h"
#include<functional>
#include<thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

//封装线程的创建和管理功能
class Thread:noncopyable
{
public:
    using ThreadFunc=std::function<void()>;  //定义线程函数的类型
    explicit Thread(ThreadFunc func,const std::string& name=std::string());  //构造函数  默认是空字符串
    ~Thread();

    void start();  //启动线程
    void join();   //等待线程结束

    bool started() const {return started_;}  //返回线程是否已经启动
    pid_t tid() const {return tid_;}  //返回线程id
    const std::string& name() const {return name_;}  //返回线程名字

    static int numCreated() {return numCreated_;}  //返回已经创建的线程数
private:
    void setDefaultName();  //设置默认线程名字
    bool started_;  //线程是否已经启动
    bool joined_;   //表示线程是否已被join()  
    std::shared_ptr<std::thread> thread_;  //线程对象  使用智能指针管理线程对象
    pid_t tid_;  //线程id
    ThreadFunc func_;  //线程函数
    std::string name_;  //线程名字
    static std::atomic_int numCreated_;  //已经创建的线程数
};