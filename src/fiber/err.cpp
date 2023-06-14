#include "internal.h"

namespace lom
{

namespace fiber
{

void SetError(const Str &s, CodePos _cp)
{
    int save_errno = errno;
    SetErr(save_errno == 0 ? s : s.Concat(Sprintf(" <errno=%d>", save_errno)), _cp);
    errno = save_errno;
}

}

}
