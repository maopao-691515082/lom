#include "internal.h"

namespace lom
{

double RandGenerator::Rand()
{
    seed_x_ = seed_x_ * 171 % 30269;
    seed_y_ = seed_y_ * 172 % 30307;
    seed_z_ = seed_z_ * 170 % 30323;
    return fmod(seed_x_ / 30269.0 + seed_y_ / 30307.0 + seed_z_ / 30323.0, 1.0);
}

uint64_t RandGenerator::RandN(uint64_t n)
{
    if (n == 0)
    {
        return 0;
    }

    /*
    由于[0,1)范围的浮点数数量不足64bit整数，且不均匀，对于比较大的n，
    这里采用分别取两个32-bit数再组合的方式缓解这个问题
    */

#define LOM_RAND_UINT32_RAND() (static_cast<uint64_t>(this->Rand() * static_cast<double>(1ULL << 32)))

    auto r = LOM_RAND_UINT32_RAND();
    return (n < kUInt32Max ? r : ((r << 32) + LOM_RAND_UINT32_RAND())) % n;
}

void RandGenerator::SRand(uint64_t seed)
{
    seed_x_ = seed % 30268;
    seed /= 30268;
    seed_y_ = seed % 30306;
    seed /= 30306;
    seed_z_ = seed % 30322;
    seed /= 30322;
    ++ seed_x_;
    ++ seed_y_;
    ++ seed_z_;
}

static RandGenerator *TLSRandGenerator()
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
    return TLSRandGenerator()->Rand();
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
