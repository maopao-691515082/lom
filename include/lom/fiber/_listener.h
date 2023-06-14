#pragma once

#include "_fd.h"
#include "_conn.h"

namespace lom
{

namespace fiber
{

class Listener : public Fd
{
public:

    /*
    接收连接，返回连接对象，如果出错，连接对象IsValid()为false
    若err_code不为nullptr，则将错误代码存入
    */
    Conn Accept(int64_t timeout_ms = -1, int *err_code = nullptr) const;

    //从一个原始fd创建新的Listener对象，如果出错，其IsValid()为false
    static Listener FromRawFd(int fd);
};

/*
监听TCP端口（IPv4）
返回Listener对象，如果出错，其IsValid()为false
*/
Listener ListenTCP(uint16_t port);

/*
监听Unix域流式socket，path必须是一个普通的文件路径，不能是空串或长度超过sockaddr_un.sun_path的大小减一
*/
Listener ListenUnixSockStream(const char *path);

/*
类似ListenUnixSockStream，但是使用Linux的抽象路径机制，输入的path不需要带首位的\0，接口会自动补上，
因此path的长度不能超过sockaddr_un.sun_path的大小减一
*/
Listener ListenUnixSockStreamWithAbstractPath(const Str &path);

}

}
