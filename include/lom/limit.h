#pragma once

#include "_internal.h"

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
本库的风格是用`ssize_t`表示长度，但一般不会用`kSSizeMax`作为最大值，而是用`2**48`这个范围，
这个范围也足够大了（256T），且在做一些运算、存储方案时可以规避溢出之类的麻烦情况
*/
static const ssize_t kSSizeSoftMax = ((ssize_t)1 << 48) - 1;

}
