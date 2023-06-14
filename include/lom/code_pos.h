#pragma once

#include "_internal.h"

#include "str.h"

namespace lom
{

class CodePos
{
    const char *file_name_;
    int line_num_;
    const char *func_name_;

public:

    CodePos(
        const char *file_name = __builtin_FILE(),
        int line_num = __builtin_LINE(),
        const char *func_name = __builtin_FUNCTION())
        : file_name_(file_name), line_num_(line_num), func_name_(func_name)
    {
    }

    bool Valid() const
    {
        return file_name_ != nullptr && line_num_ > 0 && func_name_ != nullptr;
    }

    ::lom::Str Str() const
    {
        return Valid() ?
            Sprintf("File [%s] Line [%d] Func [%s]", file_name_, line_num_, func_name_) :
            "INVALID CODE-POS";
    }

    //获取原始信息，对nullptr转为空串
    const char *FileName() const
    {
        return file_name_ ? file_name_ : "";
    }
    int LineNum() const
    {
        return line_num_;
    }
    const char *FuncName() const
    {
        return func_name_ ? func_name_ : "";
    }
};

}
