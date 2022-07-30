#pragma once

#include <stdlib.h>

#include <functional>

namespace lom
{

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

static inline void Assert(bool cond)
{
    if (!cond)
    {
        abort();
    }
}

}
