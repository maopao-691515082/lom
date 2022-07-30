#pragma once

#include <stdlib.h>
#include <stdint.h>

static_assert(sizeof(long long) == 8, "error: lom needs 64-bit `long long`");
static_assert(sizeof(size_t) == 8 && sizeof(void *) == 8, "error: lom needs 64-bit pointer and `size_t`");
static_assert(sizeof(float) == 4 && sizeof(double) == 8, "error: lom needs 32-bit `float` and 64-bit `double`");

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

static const float
    kFloatMin = 0x1p-126F,
    kFloatMax = 0x1.FFFFFEp127F;

static const double
    kDoubleMin = 0x1p-1022,
    kDoubleMax = 0x1.FFFFFFFFFFFFFp1023;

}
