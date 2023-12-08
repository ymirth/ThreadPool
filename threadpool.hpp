/*
    thread_pool: yxr 2023/12/8
    reference:
        1.https://github.com/mtrebi/thread-pool    基本思路
        2.https://github.com/progschj/ThreadPool   优化append函数
*/
#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <list>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <future>

class threadpool
{
public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();

    // add a request to pool asynchronously
    template <typename F, typename... Args>                                                  // c++11 可变参数模板
    auto append(F &&, Args &&...) -> std::future<typename std::result_of<F(Args...)>::type>; // 万能引用，可以绑定左值与右值

    threadpool(const threadpool &) = delete;
    threadpool(const threadpool &&) = delete;
    threadpool &operator=(const threadpool &) = delete;
    threadpool &operator=(const threadpool &&) = delete;

private:
    void threadFunc();

private:
    std::list<std::function<void()>> m_taskList;
    std::mutex m_mutexList;
    std::condition_variable m_cv;
    std::vector<std::shared_ptr<std::thread>> m_threads;
    bool m_stop;
};

threadpool::threadpool(int thread_number, int max_requests) : m_stop(false)
{
    if (thread_number <= 0 || max_requests <= 0)
    {
        throw std::exception();
    }

    for (int i = 0; i < thread_number; i++)
    {
        m_threads.emplace_back(std::make_shared<std::thread>(
            std::bind(&threadpool::threadFunc, this)));
    }
}

threadpool::~threadpool()
{
    {
        std::lock_guard<std::mutex> guard(m_mutexList);
        m_stop = true;
    }
    m_cv.notify_all();
    for (auto &t : m_threads)
    {
        if (t->joinable())
        {
            t->join();
        }
    }
}

void threadpool::threadFunc()
{
    std::function<void()> task;
    while (true)
    {
        {
            std::unique_lock<std::mutex> guard(m_mutexList);
            m_cv.wait(guard, [this]()
                      { return !m_taskList.empty() || m_stop; });
            if (m_stop)
                break;
            task = m_taskList.front();
            m_taskList.pop_front(); // taskPtr 的引用计数减1
        }
        task();
    }
}

template <typename F, typename... Args>
auto threadpool::append(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    // create a function with bounded parameters ready to execute
    using return_type = typename std::result_of<F(Args...)>::type; // F: function type, Args: parameter type
    // if cpp_version > c++14, use following instead
    // using return_type = std::invoke_result_t<F, Args...>;

    std::function<return_type()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

    // Encapsulate it into a shared ptr in order to be able to copy construct / assign
    // use packaged_task instead of function<void()> to support return value
    auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(func);
    {
        std::unique_lock<std::mutex> guard(m_mutexList);
        m_taskList.emplace_back([taskPtr]()
                                { (*taskPtr)(); });
    }

    m_cv.notify_one();
    return taskPtr->get_future();
}

// 1.在wrapperFunc中被lambda表达式捕获的taskPtr是一个智能指针，因此taskPtr的引用计数会加1
// 2.wrapperFunc作为一个函数对象，被放入到m_taskList中，因此taskPtr的引用计数会加1
// 3.当append函数返回时，taskPtr的引用计数会减1
// 4.当m_taskList被清空时，taskPtr的引用计数会减1

// Wrap packaged task into void function
// std::function<void()> wrapperFunc = [taskPtr]()
// {
//     (*taskPtr)();
// };