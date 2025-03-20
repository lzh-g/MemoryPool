#pragma once

#include "Common.h"

#include <mutex>

namespace Memory_Pool
{
    class CentralCache
    {
    public:
        static CentralCache &getInstance()
        {
            static CentralCache instance;
            return instance;
        }

        void *fetchRange(size_t index);
        void returnRange(void *start, size_t size, size_t bytes);

    private:
        CentralCache()
        {
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
