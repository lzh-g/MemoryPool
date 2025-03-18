#include "../include/ThreadCache.h"
#include "../include/CentralCache.h"
#include "ThreadCache.h"

namespace Memory_Pool
{
    void *Memory_Pool::ThreadCache::allocate(size_t size)
    {
        // 处理0大小的分配请求
        if (size == 0)
        {
            // 至少分配一个对齐大小
            size = ALIGNMENT;
        }

        if (size > MAX_BYTES)
        {
            // 大对象直接系统分配
            return malloc(size);
        }

        size_t index = SizeClass::getIndex(size);

        // 更新自由链表大小
        freeListSize_[index]--;

        // 检查线程
    }

} // namespace Memory_Pool
