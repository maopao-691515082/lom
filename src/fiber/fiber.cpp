#include "internal.h"

namespace lom
{

namespace fiber
{

static thread_local int64_t next_fiber_seq = 1;

//temporary for initing new fiber
static thread_local Fiber *initing_fiber;
static thread_local ucontext_t fiber_init_ret_uctx;

Fiber::Fiber(std::function<void ()> run, ssize_t stk_sz) : run_(run), finished_(false), stk_sz_(stk_sz)
{
    stk_ = new char[stk_sz_];

    seq_ = next_fiber_seq;
    ++ next_fiber_seq;

    ucontext_t uctx;
    Assert(getcontext(&uctx) == 0);
    uctx.uc_stack.ss_sp = stk_;
    uctx.uc_stack.ss_size = stk_sz_;
    uctx.uc_link = nullptr;
    makecontext(&uctx, Fiber::Start, 0);
    initing_fiber = this;
    Assert(swapcontext(&fiber_init_ret_uctx, &uctx) == 0);
}

Fiber::~Fiber()
{
    delete[] stk_;
}

void Fiber::Start()
{
    //init ctx
    if (setjmp(initing_fiber->ctx_) == 0)
    {
        //swap and never back here
        ucontext_t tmp_uctx;
        Assert(swapcontext(&tmp_uctx, &fiber_init_ret_uctx) == 0);
    }

    for (;;)    //future: for recycling
    {
        Fiber *curr_fiber = GetCurrFiber();
        curr_fiber->run_();
        curr_fiber->finished_ = true;

        curr_fiber->run_ = [] () {};
        curr_fiber->waiting_evs_.Reset();

        //to sched
        if (setjmp(curr_fiber->ctx_) == 0)
        {
            longjmp(*GetSchedCtx(), 1);
        }
    }
}

Fiber *Fiber::New(std::function<void ()> run, ssize_t stk_sz)
{
    return new Fiber(run, stk_sz);
}

void Fiber::Destroy()
{
    delete this;
}

void Create(std::function<void ()> run, ssize_t stk_sz)
{
    AssertInited();
    if (stk_sz < kStkSizeMin)
    {
        stk_sz = kStkSizeMin;
    }
    if (stk_sz > kStkSizeMax)
    {
        stk_sz = kStkSizeMax;
    }
    RegFiber(Fiber::New(run, stk_sz));
}

}

}
