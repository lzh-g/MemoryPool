#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>

namespace Memory_Pool
{
#define MEMORY_POOL_NUM 64
#define SLOT_BASE_SIZE 8
#define MAX_SLOT_SIZE 512

    // 具体内存池的槽大小没法确定，因为每个内存池的槽大小不同(8的倍数)所以这个槽结构体的sizeof不是实际的槽大小
    struct Slot
    {
        std::atomic<Slot *> next; // 原子指针
    };

    class MemoryPool
    {
    public:
        MemoryPool(size_t BlockSize = 4096);
        ~MemoryPool();

        // 初始化内存池
        void init(size_t size);

        // 分配内存
        void *allocate();
        // 释放内存
        void deallocate(void *ptr);

    private:
        // 分配新的内存块
        void allocateNewBlock();
        // 内存对齐，让指针对齐到槽大小的位置
        size_t padPointer(char *p, size_t align);
        // 使用CAS(Compare-and-Swap)操作进行无锁入队和出队
        bool pushFreeList(Slot *slot);
        Slot *popFreeList();

    private:
        int BlockSize_;    // 内存块大小
        int SlotSize_;     // 槽大小
        Slot *firstBlock_; // 指向内存池管理的首个实际内存块
        Slot *curSlot_;    // 指向当前未被使用过的槽
        // Slot *freeLists_;              // 指向空闲的槽(被使用过又被释放的槽)
        std::atomic<Slot *> freeLists_; // 指向空闲的槽(被使用过又被释放的槽)
        Slot *lastSlot_;                // 指向当前内存块中最后能存放元素的位置标识
        // std::mutex mutexForFreeLists_;  // 保证freeList_在多线程中操作的原子性
        std::mutex mutexForBlock_; // 保证多线程情况下避免不必要的重复开辟内存导致的浪费行为
    };

    class HashBucket
    {
    public:
        // 初始化内存池
        static void initMemoryPool();
        // 单例模式
        static MemoryPool &getMemoryPool(int index);

        // 分配内存
        static void *useMemory(size_t size);

        // 释放内存
        static void freeMemory(void *ptr, size_t size);

        template <typename T, typename... Args>
        friend T *newElement(Args &&...args);

        template <typename T, typename... Args>
        friend void deleteElement(T *p);
    };

    template <typename T, typename... Args>
    T *newElement(Args &&...args)
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
    void deleteElement(T *p)
    {
        // 对象析构
        if (p)
        {
            p->~T();
            // 内存回收
            HashBucket::freeMemory(reinterpret_cast<void *>(p), sizeof(T));
        }
    }
} // namespace Memory_Pool