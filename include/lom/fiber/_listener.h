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
    接收连接，返回连接对象，如果出错，连接对象`Valid()`为false
    若`err_code`不为nullptr，则将错误代码存入
    */
    Conn Accept(int64_t timeout_ms = -1, int *err_code = nullptr) const;

    /*
    调用`Serve`方法进入监听服务
        - 方法执行过程会一直循环接收链接，并根据给定的处理函数将连接指派给worker线程执行
        - `worker_count`参数：
            * 为0表示不创建worker线程，链接在本线程内部开启协程来处理
            * 若超出`kWorkerCountMax`，则调整为后者
        - `work_with_conn`函数用于回调处理新链接，由于可能在其他worker线程环境中执行，调用者自己处理好相关问题
        - 若指定`init_worker`，则它会在每个worker线程做完fiber初始化后调用
    错误和退出处理机制：
        - 只有当外部关闭Listener，或通过任何等价方式要求停止执行时，`Serve`才会退出，并返回对应的错误码
        - `Accept`接收链接失败，或传递链接给worker线程出错时`Serve`不会退出，
          而是会调用`err_log`函数（若指定），然后继续运行
            - `Accept`失败时会短暂睡眠一下，防止监听的fd出现问题时无限打印日志
            - 注意`err_log`可能从当前线程或worker线程调用，由调用者自己保证安全性
        - `Serve`退出并不会影响worker线程，各线程依然会继续执行，即便所有链接对应的协程都结束
            - 主要是因为使用fiber框架的线程在`fiber::Run()`中目前没有退出机制，
              一般来说使用`Serve`的场景也都是永久服务，就先简单处理
    */
    static const size_t kWorkerCountMax = 1024;
    int Serve(
        size_t worker_count, std::function<void (Conn)> work_with_conn,
        std::function<void (const Str &)> err_log = nullptr,
        std::function<void (size_t)> init_worker = nullptr) const;

    //从一个原始fd创建新的`Listener`对象，如果出错，其`Valid()`为false
    static Listener FromRawFd(int fd);
};

/*
监听TCP端口（IPv4）
返回`Listener`对象，如果出错，其`Valid()`为false
*/
Listener ListenTCP(uint16_t port);

/*
监听Unix域流式socket，`path`必须是一个普通的文件路径，不能是空串或长度超过`sockaddr_un.sun_path`的大小减一
*/
Listener ListenUnixSockStream(const char *path);

/*
类似`ListenUnixSockStream`，但是使用Linux的抽象路径机制，输入的`path`不需要带首位的`\0`，接口会自动补上，
因此`path`的长度不能超过`sockaddr_un.sun_path`的大小减一
*/
Listener ListenUnixSockStreamWithAbstractPath(const Str &path);

}

}
