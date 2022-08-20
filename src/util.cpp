#include "internal.h"

namespace lom
{

void Die(Str msg, CodePos _cp)
{
    fprintf(stderr, "LOM-DIE: [%s] %s\n\n", _cp.Str().CStr(), msg.CStr());
    fflush(stderr);
    kill(getpid(), SIGKILL);
}

}
