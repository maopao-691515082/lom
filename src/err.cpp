#include "internal.h"

namespace lom
{

static thread_local GoSlice<Str> err;

void SetErr(Str s, const char *file_name, int line_num, const char *func_name)
{
    err = err.Nil().Append(s);
    PushErrBT(file_name, line_num, func_name);
}

Str Err()
{
    if (err.Len() == 0)
    {
        return "";
    }
    auto s = StrSlice("\n").Join(err.Reverse());
    SetErr(s);
    return s;
}

void PushErrBT(const char *file_name, int line_num, const char *func_name)
{
    if (file_name != nullptr && line_num > 0 && func_name != nullptr)
    {
        err = err.Append(Sprintf("- File [%s] Line [%d] Func [%s]", file_name, line_num, func_name));
    }
}

}
