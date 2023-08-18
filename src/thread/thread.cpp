#include "../internal.h"

namespace lom
{

namespace thread
{

bool SetThreadName(const char *name)
{
    return prctl(PR_SET_NAME, name) != -1;
}

}

}
