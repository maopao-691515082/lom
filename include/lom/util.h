#pragma once

#include "_internal.h"

#include "str.h"
#include "code_pos.h"

namespace lom
{

//`Defer`对象在销毁时执行指定函数，一般用法是定义变量，在变量所在块结束时执行收尾工作
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

/*
打印错误信息并通过`_exit(1)`立即退出进程
注意不是普通`exit`，不会执行语言级别的收尾工作，主要是为了避免全局变量析构顺序的不确定导致的问题
如果需要收尾，请自行实现exit
*/
void Die(const Str &msg = "", CodePos _cp = CodePos());

}
