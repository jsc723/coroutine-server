#include <unistd.h>
#include <arpa/inet.h>
#include "scheduler.hpp"


namespace co_syscall {
    task accept(scheduler &sche, int __fd, sockaddr *__restrict__ __addr, socklen_t *__restrict__ __addr_len) {
        co_await sche.wait_read(__fd);
        co_return ::accept(__fd, (struct sockaddr *)&__addr, __addr_len);
    }

    task read(scheduler &sche, int __fd, void *__buf, size_t __nbytes) {
        co_await sche.wait_read(__fd);
        co_return ::read(__fd, __buf, __nbytes);
    }

    task read_exact_n(scheduler &sche, int fd, void *buf, size_t n) {
        size_t total = 0;
        while (total < n) {
            int bytes_read = co_await read(sche, fd, (char *)buf + total, n - total);

            if (bytes_read > 0) {
                total += bytes_read;
            } else if (bytes_read == 0) {
                // End of file reached before reading all bytes
                break;
            } else {
                perror("Read error");
                co_return -1;
            }
        }
        co_return total;
    }
};