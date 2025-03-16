#include "MemoryPool.h"

void *Memory_Pool::HashBucket::useMemory(size_t size)
{
    if (size <= 0)
    {
        return nullptr;
    }
    if (size > MAX_SLOT_SIZE)
    {
        // 大于512字节的内存，则使用new
        return operator new(size);
    }

    // 相当于size / 8 向上取整(因为分配内存只能大不能小)
    return getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).allocate();
}

void Memory_Pool::HashBucket::freeMemory(void *ptr, size_t size)
{
    if (!ptr)
    {
        return;
    }

    if (size > MAX_SLOT_SIZE)
    {
        operator delete(ptr);
        return;
    }

    getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).deallocate(ptr);
}

template <typename T, typename... Args>
T *Memory_Pool::newElement(Args &&...args)
{
    T *p = nullptr;
    // 根据元素大小选取合适的内存分配池分配内存
    if ((p = reinterpret_cast<T *>(HashBucket::useMemory(sizeof(T)))) != nullptr)
    {
        // 在分配的内存上构造对象
        new (p) T(std::forward<Args>(args)...);
    }

    return p;
}

template <typename T, typename... Args>
void Memory_Pool::deleteElement(T *p)
{
}