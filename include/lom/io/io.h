#pragma once

#include <lom/err.h>

namespace lom
{

namespace io
{

/*
用于标识文件读取完成（相当于read等系统调用返回0的情况）的错误
全局唯一，可直接用==比较，当然相关接口也需要遵守约定返回这个对象本身
*/
extern const Err::Ptr ErrEOF;

}

}
