## Mutex ThreadPool Implementaion
**概述**

管理一个任务队列，以及一个线程数组，每个线程每次从任务队列中选择一个任务执行。任务函数参数不限
类型以及数量，且可提交任一返回类型的任务函数。 

- 使用 `mutex` 和 `unique_lock` 以及条件变量实现线程的同步问题
- 使用万能应用以及可变参数模板，允许用户提交的任务函数的参数不限类型以及数量
- 使用 `std::function` 和 `std::bind` 以及 lambda 函数，将任务函数转换为线程函数，并生成相应的任务队列
- 使用 `packaged_task` 延迟启动和 `future` 异步获取任务的返回值


**使用方法**

```cpp
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
```



## LockFreeQueue (可用来改进ThreadPool)

**概述**

该实现提供了一个循环的无锁队列，设计用于处理多线程的并发访问，无需使用锁。

**参考**

- cpp concurrency in action
- https://gitbookcpp.llfc.club/sections/cpp/concurrent/concpp13.html

**描述**

循环无锁队列是使用原子操作和内存排序来实现的，以确保线程安全而无需锁定机制

**使用方法**

要使用循环无锁队列，请按照以下步骤操作：

- 包括必要的头文件和依赖项。
- 使用所需的容量和数据类型创建 `CircularQueue` 类的实例。
- 使用 `push` 或 `emplace` 方法将任务或元素推入队列。
- 使用 `pop(val)` 方法从队列中弹出任务或元素。

```cpp
#include<iostream>
#include<thread>
#include"lockfreequeue.hpp"

int main(){
    constexpr size_t queue_size = 100;  // 队列大小
    CircularQueue<int, queue_size> q;

    // 添加元素
    q.push(1);
    q.emplace(2);

    // 弹出元素
    while(!queue.empty()){
        int val;
        q.pop(val);
        std::cout << val << std::endl;
    }
}

```

## LockFreeThreadPool 

**概述**

基于无锁循环队列实现的任务队列，用于处理多线程的并发添加任务和执行任务，支持异步获取任务的返回值。

- 使用基于'atomic'的无锁循环队列实现任务队列
- 使用'packaged_task'延迟启动和'future'异步获取任务的返回值

**使用方法**
```cpp
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include "lockfreepool.hpp"

int main() {
    constexpr size_t queue_size = 100;  // 任务队列大小
    constexpr int thread_number = 4;    // 线程池中线程数量

    // 创建线程池
    LockFreePool<queue_size> pool(thread_number);

    // 初始化线程池
    pool.init();
    
    // 添加任务
    std::future<int> f = pool.append([](int a, int b) { return a + b; }, 1, 2);

    // 异步获取任务的返回值
    std::cout << f.get() << std::endl;

    // 关闭线程池
    return 0;
}
```

**测试**

使用LockFreePool/test.cpp进行测试


