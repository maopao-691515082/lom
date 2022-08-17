#include "internal.h"

namespace lom
{

static thread_local Str last_err;

void SetLastErr(Str s)
{
    last_err = s;
}

Str LastErr()
{
    return last_err;
}

}
