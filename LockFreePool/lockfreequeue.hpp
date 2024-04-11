/*
    2024.3.12   by yxr
    Circular Lock-free Queue
    reference:
        1. https://gitbookcpp.llfc.club/sections/cpp/concurrent/concpp13.html
        2. cpp concurrency in action   
    使用时可将注释删除
*/
#include<atomic>
#include<memory>
#include<iostream>
#include<exception>
#include<future>
#include<iostream>
#include<chrono>

template<typename T,  size_t Cap, class alloc = std::allocator<T>>
class CircularQueue: private alloc{
private:
    static constexpr size_t CACHELINE_SIZE = 64;
    // alignas(CACHELINE_SIZE) 用于告诉编译器，将head_和tail_放在不同的cache line中
    alignas(CACHELINE_SIZE) std::atomic<size_t> head_;         //head_指向的是队头
    alignas(CACHELINE_SIZE) std::atomic<size_t> tail_;         //tail_指向的是指向的是下一个要写入的位置;
    alignas(CACHELINE_SIZE) std::atomic<size_t> tail_update_;  //tail_update_ 与 tail_相同，再插入数据完成时更新tail_update_
    // tail_update_值应该与tail_相同，且不与head相同；相同时表示在向空队列中插入数据，但是数据没有更新完成
    // 所以需要检验  head_ == tail_update_ 来判断队列是否为空
    
    T* data;
    size_t max_size_;
public:
    CircularQueue():
        max_size_(Cap + 1), head_{0}, tail_{0}, tail_update_{0}
    {
        data = alloc::allocate(max_size_);
    };
     ~CircularQueue() {
        for (size_t i = head_.load(); i != tail_.load(); i = (i + 1) % max_size_) {
            alloc::destroy(data + i);
        }
        alloc::deallocate(data, max_size_);
    }
#pragma region copy and move delete
    CircularQueue(const CircularQueue&) = delete;
    CircularQueue& operator=(const CircularQueue&) = delete;
    CircularQueue(CircularQueue&&) = delete;
    CircularQueue& operator=(CircularQueue&&) = delete;
#pragma endregion

    bool empty() const{
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }


    template<typename... Args>
    bool emplace(Args&&... args){
        size_t tail;
        // 1. update tail_
        do{
            tail = tail_.load(std::memory_order_relaxed);                                               // 1  relaxed load
            size_t next_tail = (tail + 1) % max_size_;
            if(next_tail == head_.load(std::memory_order_acquire)){                                     // 2  acquire load
                return false;
            }
        }while(tail_.compare_exchange_weak(tail, (tail + 1) % max_size_, std::memory_order_release,     // 3  release store and release load
                std::memory_order_relaxed) == false);

        // 2. update data

        // 在更新完尾指针 tail_ 后再写入数据, 而将要更新数据位置存入局部变量tail中
        // 可以保证多个线程同时插入数据时，不会出现数据覆盖的情况
        alloc::construct(data + tail, std::forward<Args>(args)...);
        
        // 3. update tail_update_

        // 更新tail_update_: 保证tail_update_ == tail_( tail_ != head_ => tail_update_ != head_ )时，数据已经更新完
        size_t tailup;
        do{
            tailup = tail;
        }while(tail_update_.compare_exchange_strong(tailup,(tailup + 1) % max_size_,                    // 4 release store
                std::memory_order_release, std::memory_order_relaxed) == false);
    
        return true;
    }

    bool push(const T& value){
        return this->emplace(value);
    }
    
    bool push(T&& value){
        return this->emplace(std::move(value));
    }

    bool pop(T& value){
        T value_temp;
        size_t head;
        do{
            head = head_.load(std::memory_order_relaxed);                                               // 1 relaxed load
            if(head == tail_.load(std::memory_order_acquire)){                                          // 2 acquire load
                return false;  
            }

            // 在添加第一个数据但是未更新完数据是会出现tail_update_ == head_的情况
            //判断如果此时要读取的数据和tail_update_是否一致，如果一致说明尾部数据未更新完
            if(head == tail_update_.load(std::memory_order_acquire)){                                   // 3 acquire load
                return false;
            }

            // fix issue
            value_temp =  data[head];     // 不使用value = data[head]; 防止在没有正确读取数据时，value被修改                             
        }while(head_.compare_exchange_weak(head, (head + 1) % max_size_,                                // 4 release store and release load
                std::memory_order_release, std::memory_order_relaxed) == false);
        value = std::move(value_temp);
        return true;
    }
};
/*
    tail_update_是用来标记下一个要写入的位置，当tail_update_ == head_时，说明队列已满
    为了防止在head == tail 时, push还没更新完data[tail]的值, pop就开始读取data[head]的值
    []
    head_
    tail_
    tail_update_
    
    插入2
    
    // 更新tail_
    []              [] 
    head_           tail_  
    tail
    tail_update_

    // 写入value[tail] = 2
    2               []
    head_           tail_  
    tail
    tail_update_

    // 更新tail_update_
    2               []
    head_           tail_  
    tail            tail_update_
    
*/

/*
compare_exchange_strong操作，
    在期望的条件匹配时采用memory_order_release, 
    期望的条件不匹配时memory_order_relaxed可以提升效率(由于出于while循环中需要重试)
*/

/*
void test(){
    CircularQueue<int, 30000> queue;
   
    std::thread t1([&queue](){
        for(int i = 0; i < 10000; i++){
            queue.push(10000 + i);
        }
    });
    std::thread t2([&queue](){
        for(int i = 0; i < 10000; i++){
            queue.push(20000 + i);
        }
    });
    std::thread t3([&queue](){
        for(int i = 0; i < 10000; i++){
            queue.push(30000 + i);
        }
    });
    t1.join();
    t2.join();
    t3.join();
    

    int value;
    while(queue.pop(value)){
        std::cout << value << "\t";
    }
}
int main(){
    test();
    return 0;
}
*/