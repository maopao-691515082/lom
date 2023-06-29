#pragma once

#include "_internal.h"

namespace lom
{

//随机数生成器

//RandGenerator类非线程安全
class RandGenerator
{
    std::mt19937_64 r_;

public:

    RandGenerator(uint64_t seed) : r_(seed)
    {
    }

    //返回[0, 1)的一个随机数
    double Rand()
    {
        static const uint64_t kMod = 1ULL << 52;    //IEEE double精度
        return static_cast<double>(RandN(kMod)) / static_cast<double>(kMod);
    }

    //返回[0, n)的一个随机整数，若n为0则返回0
    uint64_t RandN(uint64_t n)
    {
        return n == 0 ? 0 : r_() % n;
    }

    //设置随机种子
    void SRand(uint64_t seed)
    {
        r_.seed(seed);
    }
};

//使用默认的thread_local的RandGenerator的快捷方式
double Rand();
uint64_t RandN(uint64_t n);
void SRand(uint64_t seed);

}
