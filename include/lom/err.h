#pragma once

#include "_internal.h"

#include "str.h"
#include "code_pos.h"

namespace lom
{

/*
设置和获取错误（thread_local存储）
PushErrBT一般在连续返回的时候做无参数调用，会将调用处的位置信息追加到错误信息前面
SetErr的位置信息可以传递(nullptr, 0, nullptr)来禁止记录当前位置，一般就是protector里面恢复时候使用
设置接口不会改变errno
*/
void SetErr(const Str &s, CodePos _cp = CodePos());
void PushErrBT(CodePos _cp = CodePos());
Str Err();

//错误保护对象，一般用在析构或Defer机制的入口处，避免自动析构机制中的错误冲掉本应返回的错误信息
class ErrProtector
{
    void *p_;

    ErrProtector(const ErrProtector &) = delete;
    ErrProtector &operator=(const ErrProtector &) = delete;

public:

    ErrProtector();
    ~ErrProtector();
};

}
