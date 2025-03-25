#include "InetAddress.h"
#include <strings.h>
#include <string.h>
InetAddress::InetAddress(uint16_t port,std::string ip = "127.0.0.1")
{
    ::memset(&addr_,0,sizeof(addr_));   //初始化置为0  之后就是addr_的结构填写
    addr_.sin_family = AF_INET;  //表示这是一个IPv4地址
    addr_.sin_port=::htons(port); // 本地字节序转为网络字节序
    addr_.sin_addr.s_addr= ::inet_addr(ip.c_str());  //设置IP地址

}

std::string InetAddress::toIp() const   //获取ip
{
    char buf[64]={0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);  //inet_ntop函数用于将网络字节序的IP地址转换为点分十进制（IPv4）
    return buf;
}

std::string InetAddress::toIpPort() const  //获取ipport
{   
    //ip:port
    char buf[64]={0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end=::strlen(buf);
    uint16_t port=::ntohs(addr_.sin_port);
    sprintf(buf+end,":%u",port);
    return buf;
}

uint16_t InetAddress::toPort() const //获取port
{
    return ::ntohs(addr_.sin_port);
}