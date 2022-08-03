#include "../internal.h"

namespace lom
{

namespace math
{

bool IsPrime(uint64_t n)
{
    if (n <= 3)
    {
        return n >= 2;
    }
    if (n % 2 == 0)
    {
        return false;
    }
    for (uint64_t i = 3; i < ((uint64_t)1 << 32) && i * i <= n; i += 2)
    {
        if (n % i == 0)
        {
            return false;
        }
    }
    return true;
}

}

}
