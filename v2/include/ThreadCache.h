#pragma once

#include "Common.h"

namespace Memory_Pool
{
    // 线程本地缓存
    class ThreadCache
    {
    public:
        // 单例模式
        static ThreadCache *getInstance()
        {
            // thread_local用于声明 线程局部存储 的变量，thread_local变量在每个线程中都有独立的实例
            static thread_local ThreadCache instance;
            return &instance;
        }

        // 分配内存
        void *allocate(size_t size);

        // 释放内存
        void deallocate(void *ptr, size_t size);

    private:
        ThreadCache()
        {
            // 初始化自由链表和大小统计
            freeList_.fill(nullptr);
            freeListSize_.fill(0);
        }

        // 从中心缓存获取内存
        void *fetchFromCentralCache(size_t index);

        // 归还内存到中心缓存
        void returnToCentralCache(void *start, size_t size);

        // 判断是否应该将内存回收给中心缓存
        bool shouldReturnToCentralCache(size_t index);

    private:
        std::array<void *, FREE_LIST_SIZE> freeList_;     // 每个线程的自由链表数组
        std::array<size_t, FREE_LIST_SIZE> freeListSize_; // 自由链表大小统计
    };

} // namespace Memory_Pool
