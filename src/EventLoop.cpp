#include "EventLoop.h"
#include "CurrentThread.h"
#include "Channel.h"
#include <sys/eventfd.h>
#include <functional>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>
#include "Logger.h"
#include "Poller.h"

//防止一个线程创建多个eventloop  指针指向eventloop对象
__thread EventLoop *t_loopInThisThread=nullptr;  

//定义默认的 poller IO复用接口超时时间
const int kPollTimeMs = 10000;

//创建wakeupfd 用来notify唤醒subreactor处理新来的channel
int createEventfd()
{
    int evtfd= ::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(evtfd<0)
    {
        LOG_FATAL("eventfd error: %d \n",errno);
    }
    return evtfd;
}
EventLoop::EventLoop()
:looping_(false)
,quit_(false)
,callingPendingFunctors_(false)
,threadId_(CurrentThread::tid())
,poller_(Poller::newDefaultPoller(this))  //返回一个epoll实例
,wakeupFd_(createEventfd())
,wakeupChannel_(new Channel(this,wakeupFd_))
{
    LOG_DEBUG("Eventloop create %p in thread %d \n",this,threadId_);
    if(t_loopInThisThread)  //说明这个线程已经有一个loop了，那就不能创建
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread=this;  //this是一个指针指向当前正在执行的成员函数所属的对象实例
    }

    //设置wakeupfd的事件类型以及发生事件后的回调事件
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));  //将EventLoop实例的handleRead成员函数绑定为wakeupChannel_的读事件回调函数
    wakeupChannel_->enableReading();// 每一个EventLoop都将监听wakeupChannel_的EPOLL读事件了

}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll(); // 给Channel移除所有感兴趣的事件，这样可以确保在销毁EventLoop对象之前，不再监听任何事件，避免在销毁过程中发生未处理的事件。
    wakeupChannel_->remove();   ////在Channel所属的EventLoop中删除Channel,也就是在poller里面删除
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

//开启事件循环
void  EventLoop::loop()
{
    looping_=true;
    quit_=false;

    LOG_INFO("EventLoop %p start looping\n",this);  //%p 用于输出指针的值。它以十六进制格式显示指针的地址。
    while(!quit_)   //开始循环 poller检测
    {
        activeChannels_.clear();
        pollRetureTime_ =poller_->poll(kPollTimeMs, &activeChannels_);  //调用poll方法
        for(Channel *channel :activeChannels_)   //遍历发生活跃的事
        {
            //Poller监听哪些channel发生了事件 然后上报给EventLoop, EventLoop通知channel处理相应的事件
            channel->handleEvent(pollRetureTime_);
        }
        
        /*
         * IO线程 mainLoop accept 会返回 fd《=channel 已连接用户的channel要分发给subloop
         * mainLoop 事先注册一个回调cb（需要subLoop来执行）  wakeup
         * mainloop调用queueInLoop将回调加入subloop（该回调需要subloop执行 但subloop还在poller_->poll处阻塞） queueInLoop通过wakeup将subloop唤醒
        */
        //执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping\n",this);
}
//退出事件循环  1.loop在自己的线程中调用quit   2. 在非loop的线程中调用loop的quit
void EventLoop::quit()
{
    quit_=true;
    if(!isInLoopThread())  //如果是在其他线程中，调用的quit  
    {
        wakeup();    //唤醒  比如在一个subloop(worker)中调用mainloop(IO)的quit时 需要唤醒mainloop(IO)的poller_->poll 让其执行完loop()函数
    }

}

//在当前loop中执行  注册回调函数
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())  //在当前的Loop线程中执行cb
    {
        cb();
    }
    else  // 在非当前EventLoop线程中执行cb，就需要唤醒EventLoop所在线程执行cb   例如你在subloop2中调用了subloop3的runInLoop
    {
        queueInLoop(cb);
    }
}

//把上层注册的回调函数cb放入队列中 唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);      //把要执行的回调操作放进去
    }
    //唤醒相应的，需要执行上面回调操作的loop操作 
    /**
     * || callingPendingFunctors的意思是 当前loop正在执行回调中 但是loop的pendingFunctors_中又加入了新的回调 需要通过wakeup写事件
     * 唤醒相应的需要执行上面回调操作的loop的线程 让loop()下一次poller_->poll()不再阻塞（阻塞的话会延迟前一次新加入的回调的执行），然后
     * 继续执行pendingFunctors_中的回调函数
     **/
    if(!isInLoopThread()||callingPendingFunctors_)
    {
        wakeup();  //唤醒loop所在线程
    }

}
void EventLoop::handleRead()  //定义回调函数
{
    uint64_t one =1;  //定义一个64位无符号整数one并将其初始化为1 （8字节）
    ssize_t n =read(wakeupFd_,&one,sizeof(one));   //read函数将读取的数据存储到one变量中，并返回读取的字节数。
    if(n!=sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
    }
}

//通过eventfd唤醒loop所在的线程（mainreactor 用来唤醒subreactor）
//向wakeupFd_写一个数据 wakeupChannel就发生读事件 当前loop线程就会被唤醒  (poller阻塞唤醒)
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n =write(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

void EventLoop::updateChannel(Channel *channel)  //调用poller的相应方法，注册fd的events事件
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)  //删除现在的channel
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)   //判断channel是否在poller中
{
    poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;   //用于存储需要执行的回调操作
    callingPendingFunctors_=true;  //表示当前正在执行回调操作

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor &functor:functors)  //表示循环中的每个元素是通过引用来访问的，这意味着不会对元素进行复制
    {
        functor();   // 执行当前loop需要执行的回调操作
    }
    
    callingPendingFunctors_=false;

} 