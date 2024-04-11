#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include "threadpool.hpp"

int main() {
    constexpr int thread_number = 4;  // 线程池中线程数量

    // 创建线程池
    threadpool pool(thread_number);

    // 初始化线程池
    pool.init();
    
    // 添加任务
    std::future<int> f = pool.append([](int a, int b) { return a + b; }, 1, 2);

    // 异步获取任务的返回值
    std::cout << f.get() << std::endl;

    // 关闭线程池
    pool.shutdown();

    return 0;
}