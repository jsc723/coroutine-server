#pragma once

#include <coroutine>
#include <iostream>
#include <utility>
#include <queue>
#include <unistd.h>

class scheduler {
public:

private:
    std::queue<std::coroutine_handle<>> ready_queue;
    
};