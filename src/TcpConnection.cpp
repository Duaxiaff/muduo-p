#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <errno.h>

#include <functional>

static EventLoop *checkLoopNodtNull(EventLoop *loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
    const std::string &nameArg,
    int sockfd,
    const InetAddress &localAddr,
    const InetAddress &peerAddr)
    :loop_(checkLoopNodtNull(loop))
    ,name_(nameArg)
    ,state_(kConnecting)
    ,reading_(true)
    ,socket_(new Socket(sockfd))
    ,channel_(new Channel(loop_,sockfd))
    ,localAddr_(localAddr)
    ,peerAddr_(peerAddr)
    ,highWaterMark_(64*1024*1024)  //64M
{
    //给channel设置相应的回调函数  poller给channel通知感兴趣的事件发生了 channel会回调相应的回调函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError,this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true); //设置TCP_KEEPALIVE选项
}
TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const void* message,int len)
{

}

void TcpConnection::handleRead(Timestamp receiveTime) //处理读事件
{
    int savedErrno=0;
    ssize_t n =inputBuffer_.readFd(socket_->fd(),&savedErrno);  //从socket中读取数据到缓冲区
    if(n>0)  //有数据到达
    {
        //已经建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage shared_from_this就是获取了TcpConnection的智能指针
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }
    else if(n==0)  //客户端断开
    {
        handleClose();
    }
    else
    {
        errno=savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite() //处理写事件
{
    if(channel_->isWriting())
    {
        int savedErrno=0;
        ssize_t n =outputBuffer_.writeFd(socket_->fd(),&savedErrno); //将缓冲区中的数据写入socket
        if(n>0)
        {
            outputBuffer_.retrieve(n);//从缓冲区读取reable区域的数据移动readindex下标
            if(outputBuffer_.readableBytes()==0)
            {
                channel_->disableWriting(); //如果缓冲区没有数据了，取消写事件
                if(writeCompleteCallback_)
                {
                    //唤醒loop_对应的thread线程，执行回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this())); //将写完成的回调放入事件循环中
                }
                if(state_==kDisconnecting)
                {
                    shutdownInLoop();  // 在当前所属的loop中把TcpConnection删除掉
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose() //处理关闭事件
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d\n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();  //关闭所有事件
    TcpConnectionPtr connPtr(shared_from_this()); //获取当前TcpConnection的智能指针
    connectionCallback_(connPtr); //执行连接的回调函数
    closeCallback_(connPtr); //执行关闭连接的回调函数
}
void TcpConnection::handleError() //处理错误事件
{

}