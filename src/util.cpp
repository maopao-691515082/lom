#include "internal.h"

namespace lom
{

void Die(const Str &msg, CodePos _cp)
{
    fprintf(stderr, "LOM-DIE: [%s] %s\n\n", _cp.Str().CStr(), msg.CStr());
    fflush(stderr);
    _exit(1);
}

}
