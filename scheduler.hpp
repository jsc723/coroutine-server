#pragma once

#include <coroutine>
#include <iostream>
#include <utility>
#include <queue>
#include <unordered_map>
#include <format>
#include "task.hpp"
#include "unistd.h"


class scheduler {
public:
    scheduler(bool enable_async_io = true) {
        if (enable_async_io) {
            schedule(co_check_io());
        }
    }

    void schedule(task &&coro_task, await_state state = await_state::schedule_next_frame) {
        emplace_coro(coro_task.get_handle(), state);
    }

    void emplace_coro(task::handler_t coro, await_state state) {
        switch(state) {
            case await_state::schedule_next_frame:
                ready_queue.emplace_back(coro);
                break;
            case await_state::schedule_now:
                ready_queue.emplace_front(coro);
                break;
            case await_state::no_schedule:
            default:
                break;
        }
    }
    
    void run() {
        while(!ready_queue.empty()) {
            if (!ready_queue.empty()) {
                auto coro = ready_queue.front();
                ready_queue.pop_front();
                std::cout << std::format("in scheduler before resume {} id={}", coro.promise().name, coro.promise().id) << std::endl;
                auto inner_coro = task::get_innermost_coro(coro);
                std::cout << std::format("inner : {} id={}", inner_coro.promise().name, inner_coro.promise().id) << std::endl;
                inner_coro.resume();
                std::cout << std::format("in scheduler after resume {} id={}", coro.promise().name, coro.promise().id) << std::endl;
                if (!coro.done()) {
                    emplace_coro(coro, coro.promise().last_await_state);
                } else {
                    coro.destroy();
                }
            }
        }
    }

    
    struct fd_awaiter {
        bool await_ready() { return false; }
        void await_suspend(task::handler_t h)
        {
            h = task::get_root_coro(h);
            wait_queue.emplace(fd, h);
            coro = h;
            original_state = h.promise().last_await_state;
            h.promise().last_await_state = await_state::no_schedule;
        }
        void await_resume() {
            coro.promise().last_await_state = original_state;
        }
        int fd;
        std::unordered_map<int, task::handler_t> &wait_queue;
        task::handler_t coro;
        await_state original_state;
    };

    auto wait_read(int fd) {
        return fd_awaiter{fd, read_wait_queue};
    }
private:
    task co_check_io() {
        co_yield "co_check_io";
        fd_set readfds;
        while (1)
        {
            FD_ZERO(&readfds);
            int maxSocket = 0;

            for(auto [fd, handle]: read_wait_queue) {
                std::cout << std::format("check {} ", fd) << std::endl;
                FD_SET(fd, &readfds);
                maxSocket = std::max(maxSocket, fd);
            }

            if (maxSocket > 0) {
                timeval timeout{};
                timeout.tv_sec = 1;
                std::cout << std::format("before select") << std::endl;
                // Wait for activity on any of the sockets
                int activity = select(maxSocket + 1, &readfds, NULL, NULL, NULL);

                if (activity == -1)
                {
                    perror("Select error");
                    exit(EXIT_FAILURE);
                }

                for(int fd = 0; fd <= maxSocket; fd++) {
                    if (FD_ISSET(fd, &readfds)) {
                        std::cout << std::format("ready: {} ", fd) << std::endl;
                        auto handle = read_wait_queue[fd];
                        read_wait_queue.erase(fd);
                        ready_queue.emplace_back(handle);
                    }
                }
            }

            co_await std::suspend_always{}; //yield to let other corotines to run
        }
    }

    std::deque<task::handler_t> ready_queue;

    std::unordered_map<int, task::handler_t> read_wait_queue;
};