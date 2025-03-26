#include "Thread.h"
#include <semaphore.h>
#include "CurrentThread.h"
static std::atomic_int numCreated_(0);  //已经创建的线程数

Thread::Thread(ThreadFunc func,const std::string& name)
:started_(false)
,joined_(false)
,tid_(0)
,func_(std::move(func))
,name_(name)
{
    setDefaultName();
}


Thread::~Thread()    //detach() 与 join() 不能同时使用
{
    if(started_ && !joined_)
    {
        thread_->detach();  // thread类提供了设置分离线程的方法 线程运行后自动销毁（非阻塞）
    }
}
void Thread::start()  //启动线程
{
    //在这个函数中注意，需要用信号量保证线程的tid值获取，如果没有信号量 主线程可能会在子线程设置 tid_ 之前继续执行，这可能导致主线程读取到一个未初始化或错误的 tid_ 值。
    started_=true;
    sem_t sem;  //信号量
    sem_init(&sem,false,0);  //初始化信号量
    //开启线程
    thread_ =std::shared_ptr<std::thread>(new std::thread([&](){
        //获取线程的tid值
        tid_=CurrentThread::tid();
        sem_post(&sem);  //信号量加1
        func_();  //开启一个新线程，专门执行该线程函数
    }));

    //这里必须等待获取上面新创建的线程的tid值  （用一个信号量解决这个问题）
    sem_wait(&sem);  //信号量减1

}
void Thread::join()   //等待线程结束
{
    joined_=true;
    thread_->join();  //等待线程结束
}

void Thread::setDefaultName()
{
    int num=++numCreated_;
    if(name_.empty())
    {
        char buf[32]={0};
        snprintf(buf, sizeof buf,"Thread%d",num);
        name_=buf;
    }
}