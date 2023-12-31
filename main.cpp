#include <iostream>
#include <format>
#include <string>
#include "task.hpp"
#include "scheduler.hpp"


scheduler::task_t test(std::string prefix, int k)
{
    for(int i = 0; i < k; i++) {
        co_await std::suspend_always{};
        std::cout << std::format("Hello from test {} {}\n", prefix, i);
    }
}

scheduler::task_t get_int(int k) {
    std::cout << std::format("get_int({}) part 1\n", k);
    co_await std::suspend_always{};
    std::cout << std::format("get_int({}) part 2\n", k);

    co_return k;
}

scheduler::task_t add_task(int x, int y) {
    int a = co_await get_int(x);
    int b = co_await get_int(y);
    std::cout << "add task done with result: " << a+b << std::endl;
    co_return a+b;
}

scheduler::task_t print_result(task<int> &&t) {
    std::cout << "print_result start" << std::endl;
    co_yield "print_result";
    int result = co_await t;
    std::cout << "print_result: " << result << std::endl;
    co_return 15;
}

task<std::string> string_task() {
    std::cout << "string_task" << std::endl;
    co_return "result";
}

task<double> add_task(double a, double b) {
    co_yield "add_task";
    std::cout << std::format("add task{} {} await", a, b) << std::endl;
    co_await std::suspend_always{};
    std::cout << std::format("add task{} {} continue", a, b) << std::endl;
    co_return a+b;
}

template<typename T>
scheduler::task_t print_any_result(task<T> &&t) {
    co_yield std::format("print_any_result {}", t.get_handle().address());
    T result = co_await t;
    std::cout << "print_any_result: " << result << std::endl;
    co_return 0;
}
 
int main()
{
    scheduler sch(false);
    //sch.schedule(test("a", 10));
    //sch.schedule(test("b", 5));
    //sch.schedule(test("c", 3));
    //sch.schedule(print_result(add_task(2,3)));
    auto t1 = string_task();
    sch.schedule(print_any_result(std::move(t1)));
    auto t2 = add_task(1.2, 3.4);
    sch.schedule(print_any_result(std::move(t2)));
    auto t3 = add_task(3.0, 4.0);
    sch.schedule(print_any_result(std::move(t3)));
    sch.run();
    return 0;
}