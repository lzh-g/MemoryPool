#include "../include/MemoryPool.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

using namespace Memory_Pool;
using namespace std::chrono;

// 计时器类
class Timer
{
public:
    Timer() : start(high_resolution_clock::now()) {}
    ~Timer() = default;

    double elapsed()
    {
        auto end = high_resolution_clock::now();
        // 转换为毫秒返回
        return duration_cast<microseconds>(end - start).count() / 1000.0;
    }

private:
    high_resolution_clock::time_point start;
};

// 性能测试类
class PerformanceTest
{
public:
    // 系统预热
    static void warmup()
    {
        std::cout << "Warming up memory systems...\n";
        // 使用 pair 来存储指针和对应的大小
        std::vector<std::pair<void *, size_t>> warmUpPtrs;

        // 预热内存池
        for (int i = 0; i < 1000; ++i)
        {
            for (size_t size : {32, 64, 128, 256, 512})
            {
                void *p = MemoryPool::allocate(size);
                warmUpPtrs.emplace_back(p, size); // 存储指针和对应的大小
            }
        }

        // 释放预热内存
        for (const auto &[ptr, size] : warmUpPtrs)
        {
            MemoryPool::dealocate(ptr, size); // 使用实际分配的大小进行释放
        }
    }

private:
    // 测试统计信息
    struct TestStats
    {
        double memPoolTime{0.0};
        double systemTime{0.0};
        size_t totalAllocs{0};
        size_t totalBytes{0};
    };
};