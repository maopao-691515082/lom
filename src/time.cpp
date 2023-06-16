#include "internal.h"

namespace lom
{

int64_t NowUS()
{
    struct timeval now;
    gettimeofday(&now, nullptr);
    return static_cast<int64_t>(now.tv_sec) * 1000000 + static_cast<int64_t>(now.tv_usec);
}

int64_t NowMS()
{
    return NowUS() / 1000;
}

int64_t NowSec()
{
    return NowUS() / 1000000;
}

double NowFloat()
{
    return static_cast<double>(NowUS()) / 1e6;
}

Str StrFTime(const char *fmt, int64_t ts_sec)
{
    time_t ts = static_cast<time_t>(ts_sec < 0 ? NowSec() : ts_sec);

    struct tm tm_r;
    localtime_r(&ts, &tm_r);

    static thread_local char buf[4 * 1024];
    strftime(buf, sizeof(buf), fmt, &tm_r);

    return buf;
}

int64_t NowClockNS()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<int64_t>(now.tv_sec) * 1000000000 + static_cast<int64_t>(now.tv_nsec);
}

int64_t NowClockUS()
{
    return NowClockNS() / 1000;
}

int64_t NowClockMS()
{
    return NowClockNS() / 1000000;
}

int64_t NowClockSec()
{
    return NowClockNS() / 1000000000;
}

double NowClockFloat()
{
    return static_cast<double>(NowClockNS()) / 1e9;
}

}
