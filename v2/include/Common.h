#pragma once
#include <array>
#include <atomic>
#include <cstddef>

namespace Memory_Pool
{
    // 对齐数和大小定义
    constexpr size_t ALIGNMENT = 8;
    constexpr size_t MAX_BYTES = 256 * 1024;                 // 自由链表节点最大的内存块大小：256KB
    constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT; // ALIGNMENT等于指针void*的大小，从0~256KB，每次+8B，共值为32768

    // 内存块头部信息
    struct BlockHeader
    {
        size_t size;       // 内存块大小
        bool inUse;        // 使用标志
        BlockHeader *next; // 指向下一个内存块
    };

    // 大小类管理
    class SizeClass
    {
    public:
        // 内存向上对齐
        static size_t roundUp(size_t bytes)
        {
            // ~(ALIGNMENT - 1)对掩码取反，用于获取ALIGNMENT的高位部分
            return (bytes += ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }

        // 根据大小获取索引
        static size_t getIndex(size_t bytes)
        {
            // 确保bytes至少为ALIGNMENT
            bytes = std::max(bytes, ALIGNMENT);
            // 向上取整后-1
            return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
        }
    };
} // namespace Memory_Pool