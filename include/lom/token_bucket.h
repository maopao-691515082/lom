#pragma once

#include "_internal.h"

namespace lom
{

//令牌桶，用浮点数计算，非法输入会被静默规整化到合法范围，调用者自行保证正确性
class TokenBucket
{
    double speed_;
    double max_value_;
    double curr_value_;
    double last_time_;

    TokenBucket(const TokenBucket &) = delete;
    TokenBucket &operator=(const TokenBucket &) = delete;

public:

    /*
    考虑浮点精度问题，初始化或重置的参数需要做一定的限制
    speed范围：[1e-6, 1e8]
    max_value范围：[0, 1e12]
    非法输入会被调整到默认值或边界
    */
    TokenBucket(double speed = 1.0, double max_value = 0.0);
    void Reset(double speed = 1.0, double max_value = 0.0);

    //尝试消费令牌，返回值指示消费是否成功，若cost<=0则不做操作立即返回true
    bool TryCost(double cost = 1.0);
};

}
