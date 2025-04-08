#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <functional>
#include <string.h>


static EventLoop *checkLoopNodtNull(EventLoop *loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
    const InetAddress &listenAddr,
    const std::string &nameArg,
    Option option)
    :loop_(checkLoopNodtNull(loop))
    ,ipPort_(listenAddr.toIpPort())
    ,name_(nameArg)
    ,acceptor_(new Acceptor(loop_,listenAddr,option == kReusePort))     //创建acceptor
    ,threadPool_(new EventLoopThreadPoll(loop_,name_))     //创建线程池
    ,connectionCallback_()
    ,messageCallback_()
    ,nextConnId_(1)
    ,started_(0)
{
    //std::placeholders::_1  占位符，可以传参数 如果绑定的函数需要参数，而这些参数在回调时才会提供，那么必须使用占位符来占位，表示这些参数将在调用时传递
    // 当有新用户连接时，Acceptor类中绑定的acceptChannel_会有读事件发生，执行handleRead()调用TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));  //设置新连接的回调函数
}

TcpServer::~TcpServer()
{
    for(auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();    // 把原始的智能指针复位 让栈空间的TcpConnectionPtr conn指向该对象 当conn出了其作用域 即可释放智能指针指向的对象
        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads) //设置线程池的线程数  subloop的数量
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() //启动服务器监听   开启tcpserver的start 相当于就是开启底层的accept的listen
{
    if(started_.fetch_add(1)==0)
    {
        threadPool_->start(threadInitCallback_);//启动线程池
        //acceptor_.get() 获得裸指针
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));   //在baseloop中执行acceptor的listen函数
    }
}

// 有一个新用户连接，acceptor会执行这个回调操作，负责将mainLoop接收到的请求连接(acceptChannel_会有读事件发生)通过回调轮询分发给subLoop去处理
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    //根据轮询算法选择一个subloop 唤醒subloop，把当前connfd封装成channel分发给subloop
    EventLoop *ioloop = threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf,sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;// 这里没有设置为原子类是因为其只在mainloop中执行 不涉及线程安全问题
    std::string connName = name_ + buf; //连接名称

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // 通过sockfd获取其绑定的本机的ip地址和端口信息 
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen =sizeof(local);
    if(::getsockname(sockfd,(sockaddr *)&local,&addrlen)<0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);
    //创建TcpConnection对象
    TcpConnectionPtr conn(new TcpConnection(ioloop,connName,sockfd,localAddr,peerAddr));
    connections_[connName]=conn; //将连接对象存储到map中
    // 下面的回调都是用户设置给TcpServer => TcpConnection的，至于Channel绑定的则是TcpConnection设置的四个，handleRead,handleWrite... 这下面的回调用于handlexxx函数中
    //TcpServer=>TcpConnection=>Channel=>poller=>notify Channel调用回调
    conn->setConnectionCallback(connectionCallback_); 
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)); //设置关闭连接的回调函数

    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn)); //在subloop中执行连接建立的回调函数
}


void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn)); 
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
        name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name()); //从map中删除连接
    EventLoop *ioloop =conn->getLoop();
    ioloop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}