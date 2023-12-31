#include <unistd.h>
#include <arpa/inet.h>
#include "scheduler.hpp"

#define MAX_PACKAGE_SIZE (1 << 10) // 1 KB

namespace co_syscall {
    result_task<int> accept(scheduler &sche, int __fd, sockaddr *__restrict__ __addr, socklen_t *__restrict__ __addr_len) {
        co_await sche.wait_read(__fd);
        int client_fd = ::accept(__fd, (struct sockaddr *)&__addr, __addr_len);
        std::cout << "system accept return fd = " << client_fd << std::endl;
        co_return client_fd;
    }

    result_task<int> read(scheduler &sche, int __fd, void *__buf, size_t __nbytes) {
        co_await sche.wait_read(__fd);
        co_return ::read(__fd, __buf, __nbytes);
    }

    //return eithor n or 0 (eof) or -1 (error)
    result_task<int> read_exact_n(scheduler &sche, int fd, void *buf, size_t n) {
        size_t total = 0;
        while (total < n) {
            int bytes_read = co_await read(sche, fd, (char *)buf + total, n - total);
            //std::cout << std::format("read_exact_n bytes_read = {}\n", bytes_read);
            if (bytes_read > 0) {
                total += bytes_read;
            } else if (bytes_read == 0) {
                // End of file reached before reading all bytes
                // treat as nothing was read
                co_return 0;
            } else {
                perror("Read error");
                co_return -1;
            }
        }
        co_return total;
    }

    result_task<int> read_package(scheduler &sche, int fd, void *payload) {
        int32_t size;
        //std::cout << std::format("read package before bytes_read(&size)\n");
        int bytes_read = co_await read_exact_n(sche, fd, &size, sizeof(int32_t));
        //std::cout << std::format("read package bytes_read={} bytes_read(&size) = {}\n", bytes_read, size);
        if (bytes_read <= 0) {
            co_return bytes_read;
        }
        if (size > MAX_PACKAGE_SIZE) {
            co_return -1;
        }
        bytes_read = co_await read_exact_n(sche, fd, payload, size);
        //std::cout << std::format("read package bytes_read(&payload) = {}\n", bytes_read);
        if (bytes_read <= 0) {
            co_return bytes_read;
        }
        co_return size;
    }
};

namespace co_rpc {
    enum CALL_ID {
        ADD_NUMBER = 1
    };
    result_task<int32_t> add_number(scheduler &sche, int fd, int32_t a, int32_t b, int &err) {
        int32_t package[] = {CALL_ID::ADD_NUMBER, a, b};
        constexpr int32_t PACKAGE_SIZE = sizeof(package);
        if(::write(fd, &PACKAGE_SIZE, sizeof(PACKAGE_SIZE)) == -1) {
            err = -1;
            co_return 0;
        }
        if(::write(fd, &package, PACKAGE_SIZE) == -1) {
            err = -1;
            co_return 0;
        }
        int32_t result;
        int bytes_read = co_await co_syscall::read_package(sche, fd, &result);
        if (bytes_read <= 0) {
            err = -1;
            co_return 0;
        }
        co_return result;
    }
}