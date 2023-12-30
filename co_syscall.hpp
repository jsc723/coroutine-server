#include <unistd.h>
#include <arpa/inet.h>
#include "scheduler.hpp"


namespace co_syscall {
    task accept(scheduler &sche, int __fd, sockaddr *__restrict__ __addr, socklen_t *__restrict__ __addr_len) {
        co_await sche.wait_read(__fd);
        //std::cout << "co_listen wait_read resume" << std::endl;
        int newSocket = ::accept(__fd, (struct sockaddr *)&__addr, __addr_len);
        co_return newSocket;
    }

    task read(scheduler &sche, int __fd, void *__buf, size_t __nbytes) {
        co_await sche.wait_read(__fd);
            // Read the incoming message
        co_return ::read(__fd, __buf, __nbytes);
    }
};