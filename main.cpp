#include <iostream>
#include <format>
#include "task.hpp"
#include "scheduler.hpp"

task<int> some_task(int input) {
    std::cout << std::format("in some_task({})", input) << std::endl;
    co_return 0;
}

task<int> get_random(int input)
{
    std::cout << std::format("in get_random({}) 1", input) << std::endl;
    co_await some_task(12);
    std::cout << std::format("in get_random({}) 2", input) << std::endl;
    co_return 4;
}
 
task<int> test()
{
    task<int> v = get_random(10);
    //task<int> u = get_random(20);
    std::cout << "in test() 1\n";
    //int x = (co_await v + co_await u);
    int x = co_await v;
    std::cout << "in test() 2\n";
    co_return x;
}
 
int main()
{
    task<int> t = test();
    std::cout << "in main() 1\n";
    t();
    std::cout << "in main() 2\n";
    int result = t.getResult();
    std::cout << result << '\n';
}