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
    while(!q.empty()){
        int val;
        q.pop(val);
        std::cout << val << std::endl;
    }
}
