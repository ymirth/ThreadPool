#include<iostream>
#include<vector>
#include<future>
#include<functional>
#include"threadpool.hpp"
#include<random>
using std::vector;
std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(-1000, 1000);
auto rnd = std::bind(dist, mt);

void simulate_hard_computation() {
  std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
}

int multiply(const int a, const int b) {
  simulate_hard_computation();
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
  return res;
}


void multiply_output(int & out, const int a, const int b) {
  simulate_hard_computation();
  out = a * b;
  std::cout << a << " * " << b << " = " << out << std::endl;
}


int multiply_return(const int a, const int b) {
  simulate_hard_computation();
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
  return res;
}


int main(void)
{
    threadpool pool(4,1000);
    vector<std::future<int>> results;
    for(int i=0;i<8;i++)
    {
        auto fu = pool.append(multiply,i,i+1);
        results.emplace_back(std::move(fu));
    }
    for(int i=0;i<8;i++)
    {
        int res{};
        auto fu = pool.append(multiply_output, std::ref(res), i,i+1);
        std::cout<<res<<std::endl;
        fu.get();
        std::cout<<res<<std::endl;
    }
    for(auto& u:results)
    {
        //std::cout<<u.get()<<std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(4000 + rnd()));
    return 0;
}
