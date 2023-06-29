#include "internal.h"

namespace lom
{

RandGenerator *TLSRandGenerator()
{
    static thread_local RandGenerator *rand_generator = nullptr;
    if (rand_generator == nullptr)
    {
        rand_generator = new RandGenerator(static_cast<uint64_t>(NowUS()) * static_cast<uint64_t>(NowUS()));
    }
    return rand_generator;
}

double Rand()
{
    return TLSRandGenerator()->Rand01<double>();
}

uint64_t RandN(uint64_t n)
{
    return TLSRandGenerator()->RandN(n);
}

void SRand(uint64_t seed)
{
    return TLSRandGenerator()->SRand(seed);
}

}
