#pragma once

#include "../_internal.h"

#include "_err.h"
#include "_fd.h"
#include "_conn.h"
#include "_listener.h"
#include "_sem.h"

namespace lom
{

namespace fiber
{

//栈大小范围[128KB, 8MB]
static const ssize_t
    kStkSizeMin = 128 * 1024,
    kStkSizeMax = 8 * 1024 * 1024;

bool IsInited();
bool Init();
void MustInit();    //init or die

/*
创建新的fiber
`run`为入口函数
`stk_sz`指定栈大小，不在范围则调整至边界值
*/
void Create(std::function<void ()> run, ssize_t stk_sz = kStkSizeMin);

//开始运行，除非出现内部错误，否则永远不退出
void Run();

void Yield();
int SleepMS(int64_t ms);

}

}
