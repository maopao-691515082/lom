#pragma once

namespace lom
{

namespace fiber
{

class Sem
{
    int64_t seq_ = -1;

public:

    bool operator<(const Sem &other) const
    {
        return seq_ < other.seq_;
    }

    bool Destroy() const;

    bool IsValid() const;

    int Acquire(size_t acquire_value = 1, int64_t timeout_ms = -1) const;
    int Release(size_t release_value = 1) const;

    static Sem New(size_t value);
};

}

}
