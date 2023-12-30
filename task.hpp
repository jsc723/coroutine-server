#pragma once

#include <coroutine>
#include <iostream>
#include <utility>

using task_result_t = int;

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
        void return_value(task_result_t value) { result = std::move(value); }
 
        task_result_t result;
        std::coroutine_handle<> previous;
        std::coroutine_handle<> next;
    };
    using handler_t = std::coroutine_handle<promise_type>;
 
    task(std::coroutine_handle<promise_type> h) : coro(h) {}
    task(const task& t) = delete;
    task &operator=(const task& t) = delete;
    task(task&& t): coro{std::move(t.coro)} {
        t.coro = nullptr;
    }
    task &operator=(task &&t) {
        if (&t != this) {
            std::swap(coro, t.coro);
        }
        return *this;
    }
    ~task() { 
    }
 
    struct awaiter
    {
        bool await_ready() { return false; }
        task_result_t await_resume() { return std::move(self.promise().result); }
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

    bool done() {
        return coro.done();
    }

    task_result_t get_result() {
        return coro.promise().result;
    }

    std::coroutine_handle<promise_type> get_handle() {
        return coro;
    }

    void destroy() {
        coro.destroy();
    }

 
private:
    std::coroutine_handle<promise_type> coro;
};