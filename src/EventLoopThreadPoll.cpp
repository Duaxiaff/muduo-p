#include "EventLoopThreadPoll.h"
#include "EventLoopThread.h"

EventLoopThreadPoll::EventLoopThreadPoll(EventLoop *baseLoop,const std::string &nameArg)
:baseLoop_(baseLoop)
,name_(nameArg)
,started_(false)
,numThreads_(0)
,next_(0)
{

}
EventLoopThreadPoll::~EventLoopThreadPoll()
{

}



void EventLoopThreadPoll::start(const ThreadInitCallback &cb)  //启动线程池
{
    started_=true;
    for(int i=0;i<numThreads_;++i)
    {
        char buf[name_.size()+32];   //给底层线程设置名字
        snprintf(buf,sizeof(buf),"%s%d",name_.c_str(),i);
        EventLoopThread *t=new EventLoopThread(cb,buf);   //创建线程
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));  //将线程对象放入线程数组
        loops_.push_back(t->startLoop());  // 底层创建线程 绑定一个新的EventLoop 并返回该loop的地址
    }

    if(numThreads_==0&&cb)   // 整个服务端只有一个线程运行baseLoop
    {
        cb(baseLoop_);
    }
}

// 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop  (好粗暴的方法哈哈哈)
EventLoop *EventLoopThreadPoll::getNextLoop()
{
    EventLoop *loop=baseLoop_;

    if(!loops_.empty())    //通过轮询获取下一个处理事件的loop
    {
        loop=loops_[next_];
        ++next_;
        if(next_>=loops_.size())
        {
            next_=0;
        }
    }

    return loop;
}

std::vector<EventLoop *> EventLoopThreadPoll::getAllLoops() //获取所有的EventLoop对象
{
    if(loops_.empty())
    {
        return std::vector<EventLoop *>(1,baseLoop_);
    }
    else
    {
        return loops_;
    }
}