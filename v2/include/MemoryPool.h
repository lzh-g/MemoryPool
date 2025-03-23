#pragma once

#include "ThreadCache.h"

namespace Memory_Pool
{
    class MemoryPool
    {
        static void *allocate(size_t size)
        {
            return ThreadCache::getInstance()->allocate(size);
        }

        static void dealocate(void *ptr, size_t size)
        {
            ThreadCache::getInstance()->deallocate(ptr, size);
        }
    };

} // namespace Memory_Pool
