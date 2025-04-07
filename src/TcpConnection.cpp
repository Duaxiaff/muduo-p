#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
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

void TcpConnection::send(const std::string &buf)
{
    if(state_==kConnected) //连接成功
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(),buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size())); //在loop线程中执行sendInLoop
        }
    }
}

/**
 * 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，而且设置了水位回调
 **/
void TcpConnection::sendInLoop(const void *data,size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len; //剩余的字节数
    bool faultError = false;
    if(state_==kDisconnected)// 之前调用过该connection的shutdown 不能再进行发送了
    {
        LOG_ERROR("disconnected, give up writing");
        return;
    }
    // 表示channel_第一次开始写数据或者缓冲区没有待发送数据
    if(!channel_->isWriting()&&outputBuffer_.readableBytes()==0)
    {
        nwrote=::write(channel_->fd(),data,len); //直接写入socket
        if(nwrote>=0) //写入成功
        {
            remaining = len - nwrote;
            if(remaining == 0 &&writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this())); //写完成的回调
            }
        }
        else  //写入失败
        {
            nwrote=0;
            if(errno!=EWOULDBLOCK)  // EWOULDBLOCK表示非阻塞情况下没有数据后的正常返回 等同于EAGAIN
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE RESET
                {
                    faultError = true;
                }
            }
        }

    }
    /**
     * 说明当前这一次write并没有把数据全部发送出去 剩余的数据需要保存到缓冲区当中
     * 然后给channel注册EPOLLOUT事件，Poller发现tcp的发送缓冲区有空间后会通知
     * 相应的sock->channel，调用channel对应注册的writeCallback_回调方法，
     * channel的writeCallback_实际上就是TcpConnection设置的handleWrite回调，
     * 把发送缓冲区outputBuffer_的内容全部发送完成
     **/
    if(!faultError && remaining>0)
    {
        // 目前发送缓冲区剩余的待发送的数据的长度
        size_t oldlen = outputBuffer_.readableBytes();
        if(oldlen+remaining>=highWaterMark_ && oldlen<highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,shared_from_this(),oldlen+remaining)); //高水位线回调
        }
        outputBuffer_.append((char*)data+nwrote,remaining); //将剩余的数据添加到缓冲区
        if(!channel_->isWriting()) //如果没有注册写事件
        {
            channel_->enableWriting(); //注册写事件
        }
    }

}

 //关闭半连接
 void TcpConnection::shutdown()
 {
    if(state_==kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this)); //在所属的loop中关闭连接
    }
 }


 void TcpConnection::shutdownInLoop() //关闭连接
 {
    if(!channel_->isWriting())  // 说明当前outputBuffer_的数据全部向外发送完成
    {
        socket_->shutdownWrite(); //关闭写半连接
    }
 }

//建立连接
void TcpConnection::connectEstablished()
{
    setState(kConnected); //设置连接状态为已连接
    channel_->tie(shared_from_this()); //将channel与TcpConnection绑定
    channel_->enableReading(); //向poller注册channel的EPOLLIN读事件
    connectionCallback_(shared_from_this()); 
}
//连接销毁
void TcpConnection::connectDestroyed() //连接断开
{
    if(state_==kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); //关闭所有事件
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); //从poller中删除channel
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
    closeCallback_(connPtr); //执行关闭连接的回调函数  执行的是TcpServer的removeConnection回调方法
}
void TcpConnection::handleError() //处理错误事件
{
    int optval;
    socklen_t optlen=sizeof(optval);
    int err=0;
    //检查与当前 TcpConnection 对应的套接字（socket）是否存在错误
    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)<0)
    {
        err=errno;
    }
    else
    {
        err=optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}