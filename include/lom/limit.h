#pragma once

//这个文件进行了所需的环境约束检查，并提供C++风格的各种常用值范围定义

#include <stdint.h>

#include <sys/types.h>

//lom用到了一些GNUC扩展的语法，需要这个支持
#ifndef __GNUC__
#   error error: lom needs GNUC
#endif

//限定只支持常见的64-bit系统和数据类型模型（LP64 or LLP64）
static_assert(sizeof(long long) == 8, "error: lom needs 64-bit `long long`");
static_assert(
    sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(ssize_t) == 8,
    "error: lom needs 64-bit pointer and `size_t` and `ssize_t`");
static_assert(
    sizeof(float) == 4 && sizeof(double) == 8,
    "error: lom needs 32-bit `float` and 64-bit `double`");

namespace lom
{

static const uint8_t kUInt8Max = 0xFF;
static const int8_t kInt8Max = 0x7F, kInt8Min = -0x80;

static const uint16_t kUInt16Max = 0xFFFF;
static const int16_t kInt16Max = 0x7FFF, kInt16Min = -0x8000;

static const uint32_t kUInt32Max = ~(uint32_t)0;
static const int32_t
    kInt32Max = (int32_t)(kUInt32Max >> 1),
    kInt32Min = ~kInt32Max;

static const uint64_t kUInt64Max = ~(uint64_t)0;
static const int64_t
    kInt64Max = (int64_t)(kUInt64Max >> 1),
    kInt64Min = ~kInt64Max;

static const size_t kSizeMax = ~(size_t)0;
static const ssize_t
    kSSizeMax = (ssize_t)(kSizeMax >> 1),
    kSSizeMin = ~kSSizeMax;

static const float
    kFloatMin = 0x1p-126F,
    kFloatMax = 0x1.FFFFFEp127F;

static const double
    kDoubleMin = 0x1p-1022,
    kDoubleMax = 0x1.FFFFFFFFFFFFFp1023;

/*
本库的风格是用ssize_t表示长度，但一般不会用kSSizeMax作为最大值，而是用2^48这个范围，
这个范围也足够大了（256T），且在做一些运算、存储方案时可以规避溢出之类的麻烦情况
*/
static const ssize_t kSSizeSoftMax = ((ssize_t)1 << 48) - 1;

}
