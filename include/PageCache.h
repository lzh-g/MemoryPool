#pragma once

#include "Common.h"

#include <map>
#include <mutex>

namespace Memory_Pool
{
    class PageCache
    {
    public:
        static const size_t PAGE_SIZE = 4096; // 页大小为4K
        static const size_t RELEASE_THRESHOLD = 128;

        static PageCache &getInstance()
        {
            static PageCache instance;
            return instance;
        }

        // 分配指定页数的span
        void *allocateSpan(size_t numPages);

        // 释放span
        void deallocateSpan(void *ptr, size_t numPages);

    private:
        PageCache() = default;

        // 向系统申请内存
        void *systemAlloc(size_t numPages);

    private:
        struct Span
        {
            void *pageAddr;  // 页起始地址
            size_t numPages; // 页数
            Span *next;      // 链表指针
        };

        // 按页数管理空闲span，不同页数对应不同Span链表
        std::map<size_t, Span *> freeSpans_;

        // 页号到span的映射，记录所有已分配的span，即所有span，用于回收
        // !!这里的已分配指的是所有正在使用的+已经释放的（在freeSpans_中记录）
        std::map<void *, Span *> spanMap_;

        std::mutex mutex_;
    };
} // namespace Memory_Pool
