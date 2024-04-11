#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include "lockfreepool.hpp"

// 任务函数，打印线程编号和任务编号
void taskFunc(int threadID, int taskID) {
    std::cout << "Thread " << threadID << " executing task " << taskID << std::endl;
    // 模拟任务执行时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 测试函数，创建线程池并提交任务
void testThreadPool(int threadID, LockFreePool<100>& pool) {
    for (int i = 0; i < 10; ++i) {
        pool.append(taskFunc, threadID, i); // 向线程池提交任务
    }
}

int main() {

    /*****************************************Test*********************************************/
    constexpr int num_threads = 4;      // 测试线程数量
    constexpr size_t queue_size = 100;  // 队列大小
    constexpr int thread_number = 4;    // 线程池中线程数量

    // 创建线程池
    LockFreePool<queue_size> pool(thread_number);
    
    // 初始化线程池
    pool.init();

    // 创建测试线程
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(testThreadPool, i, std::ref(pool)); // 向测试函数传递线程池的引用
    }

    // 等待所有测试线程完成
    for (auto& t : threads) {
        t.join();
    }

    // 等待一段时间，确保所有任务完成
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 关闭线程池
    pool.shutdown();
    
    /*****************************************Test*********************************************/
    return 0;
}


