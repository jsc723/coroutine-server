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
 
int main()
{
    scheduler sch;
    sch.schedule(test("a", 10));
    sch.schedule(test("b", 5));
    sch.schedule(test("c", 3));
    sch.run();
}