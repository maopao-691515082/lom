#pragma once

namespace lom
{

namespace fiber
{

namespace err_code
{

enum
{
    kSysCallFailed  = -1,   //兼容系统调用的风格

    //标准错误代码
    kTimeout    = -2,
    kClosed     = -3,
    kIntr       = -4,
    kInvalid    = -5,
    kOverflow   = -6,
    kConnReset  = -7,

    kStdCodeMin = -10000,   //标准错误边界，比这个值小的错误码可由用户定义（若需要的话）
};

}

}

}
