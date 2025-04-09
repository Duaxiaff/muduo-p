#include <sys/epoll.h>
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"


//定义静态常量成员变量，静态成员变量只能在类外定义
const int Channel::kNoneEvent=0;  //空事件
const int Channel::KReadEvent=EPOLLIN | EPOLLPRI;  //读事件  普通数据可读以及紧急数据可读
const int Channel::KWriteEvent=EPOLLOUT; //写事件

Channel::Channel(EventLoop *loop,int fd)
:loop_(loop),
fd_(fd),
events_(0),
revents_(0),
index_(-1),
tied_(false)
{
}

Channel::~Channel()
{}

//tie方法。
/**TcpConnection中注册了Channel对应的回调函数，传入的回调函数均为TcpConnection
 * 对象的成员方法，因此可以说明一点就是：Channel的结束一定晚于TcpConnection对象！
 * 此处用tie去解决TcpConnection和Channel的生命周期时长问题，从而保证了Channel对象能够在
 * TcpConnection销毁前销毁。如果TcpConnection先销毁，那么Channel指向TcpConnection中的回调函数指针会变成悬空指针，
 * 如果执行回调操作就会报错
 **/
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_=obj;   //用tie_记录传入的对象的共享指针，这样Channel 对象可以知道它与哪个对象绑定了。
                //tie_ 现在是一个共享指针，当绑定的对象被销毁时，由于 tie_ 是一个共享指针，它的引用计数会减少。如果这是最后一个引用，那么绑定的对象将被销毁，而 Channel 对象也会随之被销毁
    tied_=true;  //设置标识为true表明TcpConnection和Channel已经绑定
}

void Channel::update()  //当改变channel所表示的fd的events事件后,该函数负责在poller里面注册fd当前的events
{
    loop_->updateChannel(this);  //通过Channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
}

void Channel::remove()  //在Channel所属的EventLoop中删除Channel
{
    loop_->removeChannel(this);
}
void Channel::handleEvent(Timestamp receiveTime)  //处理具体的事件
{
    if(tied_) //已经绑定
    {
        std::shared_ptr<void> guard=tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);  //调用回调函数的函数。
        }
        //如果对象不存在返回空，那么说明Channel的TcpConnection对象已经不存在了，那就不要调用回调函数了
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
    
}

void Channel::handleEventWithGuard(Timestamp receiveTime)//调用回调函数的函数  处理具体的事件
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);  //输出日志，正在处理具体某种事件

    //具体事件有读、写、关闭、错误
    //写
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
    //读
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    //错误
    if(revents_ & EPOLLERR) //EPOLLERR表示发生错误
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }
    //关闭
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))  //挂起并且没有数据可读
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }
}