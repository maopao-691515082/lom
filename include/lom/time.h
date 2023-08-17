#pragma once

#include "_internal.h"

#include "str.h"

namespace lom
{

/*
Now系列函数后缀说明：
`NS`：纳秒
`US`：微秒
`MS`：毫秒
`Sec`：秒
`Float`：浮点数表示的秒（考虑浮点精度，最多精确到微秒左右）
*/

//不带TimeZone的Unix时间
int64_t NowUS();
int64_t NowMS();
int64_t NowSec();
double NowFloat();

/*
`strftime`的便捷版本，`fmt`语法同`strftime`，`ts_sec`为单位为秒的时间戳，若<0则取当前时间
输出结果不能超过4KB，否则截断
*/
Str StrFTime(const char *fmt, int64_t ts_sec = -1) __attribute__((format(gnu_strftime, 1, 0)));

//monotonic clock时间
int64_t NowClockNS();
int64_t NowClockUS();
int64_t NowClockMS();
int64_t NowClockSec();
double NowClockFloat();

}
