#pragma once

#include <experimental\coroutine>

namespace ndtech {
    class VoidCoroutine
    {
    public:
        struct promise_type
        {
            VoidCoroutine get_return_object();
            std::experimental::suspend_never initial_suspend();
            std::experimental::suspend_never final_suspend();
            void return_void();
            void unhandled_exception();
        };
    };
}