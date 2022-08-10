#pragma once

#include <stdlib.h>

#include <functional>

namespace lom
{

//Defer对象在销毁时执行指定函数，一般用法是定义变量，在变量所在块结束时执行收尾工作
class Defer
{
    std::function<void ()> f_;

public:

    Defer(std::function<void ()> f) : f_(f)
    {
    }

    ~Defer()
    {
        f_();
    }
};

static inline void Assert(bool cond)
{
    if (!cond)
    {
        abort();
    }
}

//不推荐直接使用_Die，用宏LOM_DIE
void _Die(const char *file_name, int line, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

}

/*
打印错误信息并通过SIGKILL立即退出进程
注意不是普通exit，不会执行语言级别的收尾工作，主要是为了避免全局变量析构顺序的不确定导致的问题
如果需要收尾，请自行实现exit
*/
#define LOM_DIE(_fmt, _args...) ::lom::_Die(__FILE__, __LINE__, _fmt, ##_args)
