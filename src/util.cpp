#include "internal.h"

namespace lom
{

void _Die(const char *file_name, int line, const char *fmt, ...)
{
    fprintf(stderr, "LOM-DIE: [%s:%d] ", file_name, line);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n\n");
    fflush(stderr);

    kill(getpid(), SIGKILL);
}

}
