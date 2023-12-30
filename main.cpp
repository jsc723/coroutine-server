#include <iostream>
#include <format>
#include <string>
#include "task.hpp"
#include "scheduler.hpp"


task test(std::string prefix, int k)
{
    for(int i = 0; i < k; i++) {
        co_await std::suspend_always{};
        std::cout << std::format("Hello from test {} {}\n", prefix, i);
    }
}

task get_int(int k) {
    co_return k;
}

task add_task(int x, int y) {
    int a = co_await get_int(x);
    int b = co_await get_int(y);
    co_return a+b;
}

task print_result(task &&t) {
    int result = co_await t;
    std::cout << "print_result: " << result << std::endl;
}
 
int main()
{
    scheduler sch;
    sch.schedule(test("a", 10));
    sch.schedule(test("b", 5));
    sch.schedule(test("c", 3));
    sch.schedule(print_result(add_task(2,3)));
    sch.run();
}