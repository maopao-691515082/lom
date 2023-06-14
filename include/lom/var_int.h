#pragma once

#include "_internal.h"

#include "str.h"
#include "io/io.h"

namespace lom
{

namespace var_int
{

/*
将整数编码为变长的串
支持范围为int64的最小负值到uint64的最大值，范围跨越两个类型，但具体解码的时候，
输入编码必须在指定的类型范围中，否则视为编码格式错误

编码方式是自研算法，保证了编码后的结果的字典序和源值的大小序一致，并保证了常用整数（0附近）的编码尽量短
*/

Str Encode(int64_t n);
Str EncodeUInt(uint64_t n);

/*
Decode函数在成功时会调整p和sz，消费掉本次解码的长度
失败时，p和sz不作调整，n的值未定义
*/
bool Decode(const char *&p, ssize_t &sz, int64_t &n);
bool DecodeUInt(const char *&p, ssize_t &sz, uint64_t &n);

//从buf reader中读取
bool LoadFrom(const io::BufReader::Ptr &br, int64_t &n);
bool LoadUIntFrom(const io::BufReader::Ptr &br, uint64_t &n);

}

}
