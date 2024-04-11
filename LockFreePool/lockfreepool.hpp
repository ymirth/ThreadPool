#pragma once

#include<vector>
#include<memory>
#include<thread>
#include<future>
#include<atomic>
#include<functional>

#include"lockfreequeue.hpp"

template<size_t queue_size_>
class LockFreePool
{
public:
    LockFreePool(int thread_number = 8);
    ~LockFreePool();

    // add a request to pool asynchronously
    template <typename F, typename... Args>                 // c++11 variadic template
    decltype(auto) append(F &&, Args &&...) ;               // Universal reference for perfect forwarding

    void init();       // initialize thread pool
    void shutdown();   // shutdown thread pool

#pragma region delete copy and move
    LockFreePool(const LockFreePool &) = delete;
    LockFreePool(const LockFreePool &&) = delete;
    LockFreePool &operator=(const LockFreePool &) = delete;
    LockFreePool &operator=(const LockFreePool &&) = delete;
#pragma endregion

private:
    void threadFunc(); // loop function for each thread
private:
    std::vector<std::shared_ptr<std::thread>> threads_;
    std::atomic<bool> stop_;
    int thread_number_;
    CircularQueue<std::function<void()>, queue_size_> queue_;  
};

template<size_t queue_size_>
LockFreePool<queue_size_>::LockFreePool(int thread_number)
    : stop_(false), thread_number_(thread_number), queue_()
{
}

template<size_t queue_size_>
LockFreePool<queue_size_>::~LockFreePool()
{
    shutdown();
}

template<size_t queue_size_>
void LockFreePool<queue_size_>::init()
{
    if(stop_.exchange(false)) return;
    for (int i = 0; i < thread_number_; ++i)
    {
        threads_.emplace_back(std::make_shared<std::thread>(&LockFreePool<queue_size_>::threadFunc, this));
    }
}

template<size_t queue_size_>
void LockFreePool<queue_size_>::shutdown()
{
    if(!stop_.exchange(true)) return;
    for (auto &thread : threads_)
    {
        thread->join();
        thread.reset();
    }
    threads_.clear();
}

template<size_t queue_size_>
template <typename F, typename... Args>
decltype(auto) LockFreePool<queue_size_>::append(F &&f, Args &&... args)
{
    auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);             // （f绑定args...）
    auto task_ptr = std::make_shared<std::packaged_task<decltype(func())()>>(func);

    if(stop_)
    {
        return std::future<decltype(func())>();
    }

    while (!queue_.emplace([task_ptr]() { (*task_ptr)(); })){} // lock free push
    
    auto result = task_ptr->get_future();
    return result;
}

template<size_t queue_size_>
void LockFreePool<queue_size_>::threadFunc()
{
    while (!stop_)
    {
        std::function<void()> task;     
        if (queue_.pop(task))                                   // lock free pop
        {
            task();
        }
    }
}

