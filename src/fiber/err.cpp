#include "internal.h"

namespace lom
{

namespace fiber
{

void SetError(Str s, CodePos _cp)
{
    int save_errno = errno;
    if (save_errno != 0)
    {
        s = s.Concat(Sprintf(" <errno=%d>", errno));
    }
    SetErr(s, _cp);
    errno = save_errno;
}

}

}
