#pragma once

#include <coroutine>
#include <iostream>
#include <utility>

template<class T>
struct task
{
    struct promise_type
    {
        auto get_return_object()
        {
            return task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_always initial_suspend() { return {}; }
        struct final_awaiter
        {
            bool await_ready() noexcept { return false; }
            void await_resume() noexcept {}
            std::coroutine_handle<>
                await_suspend(std::coroutine_handle<promise_type> h) noexcept
            {
                // final_awaiter::await_suspend is called when the execution of the
                // current coroutine (referred to by 'h') is about to finish.
                // If the current coroutine was resumed by another coroutine via
                // co_await get_task(), a handle to that coroutine has been stored
                // as h.promise().previous. In that case, return the handle to resume
                // the previous coroutine.
                // Otherwise, return noop_coroutine(), whose resumption does nothing.
 
                if (auto previous = h.promise().previous; previous){
                    return previous;
                } 
                return std::noop_coroutine();
            }
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void unhandled_exception() { throw; }
        void return_value(T value) { result = std::move(value); }
 
        T result;
        std::coroutine_handle<> previous;
        std::coroutine_handle<> next;
    };
 
    task(std::coroutine_handle<promise_type> h) : coro(h) {}
    task(task&& t) = delete;
    ~task() { coro.destroy(); }
 
    struct awaiter
    {
        bool await_ready() { return false; }
        T await_resume() { return std::move(self.promise().result); }
        auto await_suspend(auto h)
        {
            self.promise().previous = h;
            h.promise().next = self;

            return self;
        }
        std::coroutine_handle<promise_type> self;
    };
    awaiter operator co_await() { return awaiter{coro}; }

    void operator()() {
        coro.resume();
    }

    T getResult() {
        return coro.promise().result;
    }

 
private:
    std::coroutine_handle<promise_type> coro;
};