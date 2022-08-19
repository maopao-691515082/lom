#pragma once

#include "_internal.h"

#include "str.h"

namespace lom
{

//设置和获取错误（thread_local存储）
void SetLastErr(Str s);
Str LastErr();

//错误保护对象，一般用在析构或Defer机制的入口处，避免自动析构机制中的错误冲掉本应返回的错误信息
class ErrProtector
{
    Str s_;

public:

    ErrProtector() : s_(LastErr())
    {
    }

    ~ErrProtector()
    {
        SetLastErr(s_);
    }
};

}
