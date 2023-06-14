#include "internal.h"

namespace lom
{

namespace fiber
{

bool Sem::Destroy() const
{
    return UnregSemFromSched(*this);
}

bool Sem::Valid() const
{
    return seq_ >= 0 && IsSemInSched(*this);
}

int Sem::Acquire(uint64_t acquire_value, int64_t timeout_ms) const
{
    if (!Valid())
    {
        SetError("invalid sem");
        return err_code::kInvalid;
    }

    int64_t expire_at = timeout_ms < 0 ? -1 : NowMS() + timeout_ms;

    /*
    循环acquire直到需要的acquire_value都申请完成，或到sem被销毁、超时失败等情况
    需要注意的是如果超时，则需返还done_value
    */

    uint64_t done_value = 0;
    while (acquire_value > done_value)
    {
        done_value += TryAcquireSem(*this, acquire_value - done_value);
        if (done_value == acquire_value)
        {
            return 0;
        }

        //没有扣完，需要阻塞等待
        Assert(done_value < acquire_value);
        if (expire_at >= 0 && expire_at <= NowMS())
        {
            if (done_value > 0)
            {
                //已经申请了一部分了，返还
                RestoreAcquiringSem(*this, done_value);
            }
            SetError("timeout");
            return err_code::kTimeout;
        }

        WaitingEvents evs;
        evs.expire_at_ = expire_at;
        evs.waiting_sems_.emplace_back(*this);
        SwitchToSchedFiber(evs);

        if (!Valid())
        {
            SetError("sem destroyed by another fiber");
            return err_code::kClosed;
        }
    }

    return 0;
}

int Sem::Release(uint64_t release_value) const
{
    return ReleaseSem(*this, release_value);
}

Sem Sem::New(uint64_t value)
{
    static thread_local int64_t next_sem_seq = 1;

    Sem sem;
    sem.seq_ = next_sem_seq;
    ++ next_sem_seq;
    RegSemToSched(sem, value);
    return sem;
}

}

}
