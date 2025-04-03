#include "TcpServer.h"
#include "Logger.h"


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
    Option option = kNoReusePort)
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


//根据轮询算法选择一个subloop 唤醒subloop，把当前connfd封装成channel分发给subloop
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioloop = threadPool_->getNextLoop();
}


void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{

}