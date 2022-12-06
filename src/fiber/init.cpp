#include "internal.h"

namespace lom
{

namespace fiber
{

static thread_local bool inited = false;

bool IsInited()
{
    return inited;
}

bool Init()
{
    if (!inited)
    {
        inited = InitFdEnv() && InitSched();
    }
    return inited;
}

void MustInit()
{
    if (!Init())
    {
        Die(Str("lom::fiber::MustInit: init fiber env failed: ").Concat(Err()));
    }
}

void AssertInited()
{
    Assert(inited);
}

}

}
