#pragma once

namespace lom
{

/*
随机数生成器相关库，算法：Wichmann-Hill

论文：
Brian Wichmann, David Hill,
Algorithm AS 183: An Efficient and Portable Pseudo-random Number Generator,
Applied Statistics,
Volume 31, Number 2, 1982, pages 188-190.
*/

//RandGenerator类非线程安全
class RandGenerator
{
    uint64_t seed_x_, seed_y_, seed_z_;

public:

    RandGenerator(uint64_t seed)
    {
        SRand(seed);
    }

    //返回[0, 1)的一个随机数
    double Rand();

    //返回[0, n)的一个随机整数，若n为0则返回0
    uint64_t RandN(uint64_t n);

    //设置随机种子
    void SRand(uint64_t seed);
};

//使用默认的thread_local的RandGenerator的快捷方式
double Rand();
uint64_t RandN(uint64_t n);
void SRand(uint64_t seed);

}
