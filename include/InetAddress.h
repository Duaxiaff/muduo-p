#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
//封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port,std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)  //使用引用，避免拷贝
    :addr_(addr)
    {

    }

    std::string toIp() const;   //获取ip
    std::string toIpPort() const;  //获取ipport
    uint16_t toPort() const; //获取port

    const sockaddr_in* getSockAddr() const {return &addr_;}   //返回存储的地址信息
    void setSockAddr(const sockaddr_in &addr){addr_=addr;}


private:
    sockaddr_in addr_;   //用于存储ipv4地址信息
};