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

    bool Valid() const;

    int Acquire(uint64_t acquire_value = 1, int64_t timeout_ms = -1) const;
    int Release(uint64_t release_value = 1) const;

    static Sem New(uint64_t value);
};

}

}
