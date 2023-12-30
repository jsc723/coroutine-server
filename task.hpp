#pragma once

#include <coroutine>
#include <iostream>
#include <utility>

using task_result_t = int;

enum class await_state {
    schedule_next_frame,
    schedule_now,
    no_schedule
};
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
                    previous.promise().next = nullptr;
                    //std::cout << "final suspend return previous\n";
                    return previous;
                }
                //std::cout << "final suspend return noop\n";
                return std::noop_coroutine();
            }
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void unhandled_exception() { throw; }
        void return_value(task_result_t value) { result = std::move(value); }

        await_state get_last_await_state() { return last_await_state; }

        task_result_t result;
        std::coroutine_handle<promise_type> previous;
        std::coroutine_handle<promise_type> next;
        await_state last_await_state = await_state::schedule_next_frame;
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
        task_result_t await_resume() { 
            task_result_t res = std::move(self.promise().result);
            //std::cout << "destroy coro and get result " << res  << std::endl;
            self.destroy();
            return res;
        }
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

    static handler_t get_innermost_coro(handler_t coro) {
        int advanced = 0;
        while(coro.promise().next) {
            coro = coro.promise().next;
            advanced++;
        }
        //std::cout << std::format("advanced {} times\n", advanced);
        return coro;
    }

 
private:
    std::coroutine_handle<promise_type> coro;
};