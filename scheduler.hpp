#pragma once

#include <coroutine>
#include <iostream>
#include <utility>
#include <queue>
#include "task.hpp"

class scheduler {
public:
    void schedule(task &&coro_task) {
        ready_queue.emplace(std::move(coro_task));
    }
    
    void run() {
        while(!ready_queue.empty()) {
            if (!ready_queue.empty()) {
                task &&t = std::move(ready_queue.front());
                ready_queue.pop();
                t();
                if (!t.done()) {
                    ready_queue.emplace(std::move(t));
                } else {
                    t.destroy();
                }
            }
        }
    }
private:
    std::queue<task> ready_queue;

};