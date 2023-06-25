#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <ctype.h>
#include <malloc.h>
#include <math.h>

#include <vector>
#include <initializer_list>
#include <utility>
#include <functional>
#include <type_traits>
#include <list>
#include <map>
#include <atomic>
#include <string>
#include <memory>

#include <sys/types.h>

#ifndef __GNUC__
#   error error: lom needs GNUC
#endif

static_assert(CHAR_BIT == 8, "error: lom needs 8-bit `char`");
static_assert(
    sizeof(short) == 2 && sizeof(int) == 4 && sizeof(long) == 8 && sizeof(long long) == 8,
    "error: lom needs 16-bit `short`, 32-bit `int` and 64-bit `long` & `long long`");
static_assert(
    sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(ssize_t) == 8,
    "error: lom needs 64-bit pointer and `size_t` and `ssize_t`");
static_assert(
    sizeof(float) == 4 && sizeof(double) == 8,
    "error: lom needs 32-bit `float` and 64-bit `double`");

namespace lom
{

static inline void Assert(bool cond)
{
    if (!cond)
    {
        abort();
    }
}

}
