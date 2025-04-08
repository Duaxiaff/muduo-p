#include <string>
#include "Logger.h"
#include "TcpServer.h"

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string &name)
    :loop_(loop),
    server_(loop,addr,name)
    {
        //注册回调函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnevtion,this,std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

        //设置合适的subloop的线程数量
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();  //开启服务器
    }
private:
    //连接建立或者断开的回调函数
    void onConnevtion(const TcpConnectionPtr& conn)
    {
        if(conn->connected())  //连接成功
        {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }

    }
    //可读写事件回调
    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();  //获取缓冲区中的数据
        conn->send(msg);
    }

    EventLoop* loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8080);  //端口号是8080 ip是默认的"127.0.0.1"
    EchoServer server(&loop,addr,"EchoServer");
    server.start();    //相当于是启动的listen
    loop.loop();  //开启事件循环
    return 0;
}