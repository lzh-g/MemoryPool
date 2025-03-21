#pragma once

#include "Common.h"

#include <mutex>

namespace Memory_Pool
{
    class CentralCache
    {
    public:
        // 单例模式
        static CentralCache &getInstance()
        {
            static CentralCache instance;
            return instance;
        }

        // 从中心缓存空闲链表获取一批内存块
        void *fetchRange(size_t index);
        // 返回一批内存块至中心缓存空闲链表
        void returnRange(void *start, size_t size, size_t index);

    private:
        // 初始化所有原子指针为nullptr
        CentralCache()
        {
            for (auto &ptr : centralFreeList_)
            {
                ptr.store(nullptr, std::memory_order_relaxed);
            }
            // 初始化所有锁
            for (auto &lock : locks_)
            {
                lock.clear();
            }
        }

        // 从页缓存获取内存
        void *fetchFromPageCache(size_t size);

    private:
        // 中心缓存的自由链表
        std::array<std::atomic<void *>, FREE_LIST_SIZE> centralFreeList_;

        // 用于同步的自旋锁
        std::array<std::atomic_flag, FREE_LIST_SIZE> locks_;
    };

} // namespace Memory_Pool
