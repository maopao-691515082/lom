#pragma once

#include "_internal.h"

namespace lom
{

//随机数生成器

//注：`RandGenerator`类非线程安全
class RandGenerator
{
    std::mt19937_64 r_;

public:

    RandGenerator(uint64_t seed) : r_(seed)
    {
    }

    //返回[0, 1)的一个随机数，需要指定浮点类型来计算
    template <typename T, std::enable_if_t<std::is_floating_point_v<T>> * = nullptr>
    T Rand01()
    {
        static const int kMantissaDigits = std::numeric_limits<T>::digits;
        static_assert(kMantissaDigits > 8, "error: invalid mantissa digits");
        static const uint64_t kMod = static_cast<uint64_t>(1) << std::min(63, kMantissaDigits);
        return static_cast<T>(RandN(kMod)) / static_cast<T>(kMod);
    }

    //返回[0, n)的一个随机整数，若`n`为0则返回0
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

//返回当前线程对应的`RandGenerator`对象
RandGenerator *TLSRandGenerator();

//使用默认的thread_local的`RandGenerator`的常用算法的快捷方式
double Rand();  //`Rand01<double>`
uint64_t RandN(uint64_t n);
void SRand(uint64_t seed);

}
