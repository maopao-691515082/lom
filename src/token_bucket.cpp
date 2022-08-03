#include "internal.h"

namespace lom
{

TokenBucket::TokenBucket(double speed, double max_value)
{
    Reset(speed, max_value);
}

void TokenBucket::Reset(double speed, double max_value)
{
    if (isnan(speed))
    {
        speed = 1.0;
    }
    if (isnan(max_value))
    {
        max_value = 0.0;
    }
    speed_ = std::min(std::max(speed, 1e-6), 1e8);
    max_value_ = std::min(std::max(max_value, 0.0), 1e12);
    curr_value_ = max_value_;
    last_time_ = NowFloat();
}

bool TokenBucket::TryCost(double cost)
{
    if (cost <= 0.0)
    {
        return true;
    }

    double now = NowFloat();
    if (now > last_time_)
    {
        curr_value_ += speed_ * (now - last_time_);
        last_time_ = now;
    }
    if (curr_value_ > max_value_)
    {
        curr_value_ = max_value_;
    }
    if (curr_value_ < 0)
    {
        return false;
    }
    curr_value_ -= cost;
    return true;
}

}
