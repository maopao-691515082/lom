#pragma once

#include "_fd.h"

namespace lom
{

namespace fiber
{

class Conn : public Fd
{
public:

    /*
    读数据，`sz`必须>0
    返回值：
        >0：读到的字节数
        0：文件结束
        -1：系统调用错误，可使用`errno`
        <-1：`err_code`中定义的内部错误码
    */
    ssize_t Read(char *buf, ssize_t sz, int64_t timeout_ms = -1) const;

    /*
    写数据，`sz`必须>=0，`sz`为0时仅做有效性检查，`sz`>0时允许部分成功（写入至少1字节就会成功返回）
    返回值：
        >0：成功写入的字节数
        0：仅当`sz`为0且通过了有效性检查时返回，表示成功
        -1：系统调用错误，可使用`errno`
        <-1：`err_code`中定义的内部错误码
    */
    ssize_t Write(const char *buf, ssize_t sz, int64_t timeout_ms = -1) const;

    /*
    写数据，成功则保证写完，`sz`必须>=0，`sz`为0时仅做有效性检查
    返回值：
        0：成功
        -1：系统调用错误，可使用`errno`
        <-1：`err_code`中定义的内部错误码
    */
    int WriteAll(const char *buf, ssize_t sz, int64_t timeout_ms = -1) const;

    //从一个原始fd创建新的`Conn`对象，如果出错，其`Valid()`为false
    static Conn FromRawFd(int fd);
};

/*
向地址`ip:port`建立TCP连接，IPV4版本
    `ipv4`必须是标准的IPV4格式，不支持hostname或域名
    `port`指定端口
    `timeout_ms`指定超时时间
返回连接对象，如果出错，连接对象`Valid()`为false
若`err_code`不为nullptr，则将错误代码存入
*/
Conn ConnectTCP(const char *ipv4, uint16_t port, int64_t timeout_ms = -1, int *err_code = nullptr);

/*
向本地Unix域的流式socket建立连接
`path`指定需要连接的地址，注意是普通C字符串，即`\0`结尾，长度不能超过`sockaddr_un.sun_path`的大小减一
其他输入参数含义和返回行为同`ConnectTCP`
*/
Conn ConnectUnixSockStream(const char *path, int64_t timeout_ms = -1, int *err_code = nullptr);

/*
类似`ConnectUnixSockStream`，但是使用Linux的抽象路径机制，输入的`path`不需要带首位的`\0`，接口会自动补上，
因此`path`的长度不能超过`sockaddr_un.sun_path`的大小减一
其他输入参数含义和返回行为同`ConnectUnixSockStream`
*/
Conn ConnectUnixSockStreamWithAbstractPath(const Str &path, int64_t timeout_ms = -1, int *err_code = nullptr);

}

}
