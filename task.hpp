#pragma once

#include <coroutine>
#include <iostream>
#include <utility>
#include <format>
#include <cstdint>


enum class await_state {
    schedule_next_frame,
    schedule_now,
    no_schedule
};

template<typename T>
struct task
{
    struct promise_type
    {
        static uint64_t get_next_id() {
            static uint64_t next_id{};
            return next_id++;
        }
        task<T> get_return_object()
        {
            return task<T>(std::coroutine_handle<promise_type>::from_promise(*this));
        }
        void return_value(T value) { 
            //std::cout << "task::promise write result: " << value << std::endl;
            //std::cout << "promise address = " << this << std::endl;
            result = std::move(value); 
        }
        std::suspend_always initial_suspend() { return {}; }
        struct final_awaiter
        {
            bool await_ready() noexcept { return false; }
            void await_resume() noexcept {}
            std::coroutine_handle<>
                await_suspend(auto h) noexcept
            {
                // final_awaiter::await_suspend is called when the execution of the
                // current coroutine (referred to by 'h') is about to finish.
                // If the current coroutine was resumed by another coroutine via
                // co_await get_task(), a handle to that coroutine has been stored
                // as h.promise().previous. In that case, return the handle to resume
                // the previous coroutine.
                // Otherwise, return noop_coroutine(), whose resumption does nothing.

                if (auto previous = h.promise().previous; previous){
                    //previous.promise().next = nullptr;
                    //std::cout << "final suspend return previous\n";
                    return previous;
                }
                //std::cout << "final suspend return noop\n";
                return std::noop_coroutine();
            }
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void unhandled_exception() { throw; }
        
        T get_result() {
            return coro.promise().result;
        }

        auto yield_value(std::string coro_name) {
            std::cout << "yield_value: " << coro_name << std::endl;
            name = std::move(coro_name);
            return std::suspend_never{};
        }

        await_state get_last_await_state() { return last_await_state; }

        std::coroutine_handle<promise_type> previous;
        std::coroutine_handle<promise_type> next;
        await_state last_await_state = await_state::schedule_next_frame;
        uint64_t id = get_next_id();
        std::string name;

        T result;
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
        T await_resume() { 
            //std::cout << "auto hdl = to_typed_handle<T>(self);"<< std::endl;
            self.promise().previous.promise().next = nullptr;
            T res = self.promise().result;
            //std::cout << "destroy coro and get result " << res << std::endl;
            self.destroy();
            return res;
        }
        auto await_suspend(auto h)
        {
            //std::cout << "h.address = " << h.address() << std::endl;
            handle_assign(self.promise().previous, h);
            handle_assign(h.promise().next, self);
            //std::cout << "self.promise().previous.address = " << self.promise().previous.address()<< std::endl;
            //std::cout << "h.promise().next.address = " << h.promise().next.address()<< std::endl;
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

    std::coroutine_handle<promise_type> get_handle() {
        return coro;
    }

    void destroy() {
        coro.destroy();
    }

    
 
protected:
    std::coroutine_handle<promise_type> coro;
};

auto get_innermost_coro(auto coro) {
    int advanced = 0;
    while(coro.promise().next) {
        handle_assign(coro, coro.promise().next);
        advanced++;
    }
    //std::cout << std::format("advanced {} times\n", advanced);
    return coro;
}

auto get_root_coro(auto coro) {
    int advanced = 0;
    while(coro.promise().previous) {
        handle_assign(coro, coro.promise().previous);
        advanced++;
    }
    //std::cout << std::format("go back {} times\n", advanced);
    return coro;
}

template<typename U, typename V>
void handle_assign(std::coroutine_handle<U> &to, std::coroutine_handle<V> &from)
{ 
    to = std::coroutine_handle<U>::from_address(from.address()); 
}

