#include "pch.h"

#include "VoidCoroutine.h"

namespace ndtech {
    VoidCoroutine VoidCoroutine::promise_type::get_return_object() { return {}; }
    std::experimental::suspend_never VoidCoroutine::promise_type::initial_suspend() { return {}; }
    std::experimental::suspend_never VoidCoroutine::promise_type::final_suspend() { return {}; }
    void VoidCoroutine::promise_type::return_void() {}
    void VoidCoroutine::promise_type::unhandled_exception() {}
}
