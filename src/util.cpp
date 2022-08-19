#include "internal.h"

namespace lom
{

void Die(Str msg, const char *file_name, int line_num)
{
    fprintf(stderr, "LOM-DIE: [%s:%d] %s\n\n", file_name, line_num, msg.CStr());
    fflush(stderr);
    kill(getpid(), SIGKILL);
}

}
