#include "../include/CentralCache.h"

#include <cassert>
#include <thread>
#include "CentralCache.h"

namespace Memory_Pool
{

    // 每次从PageCache获取span大小（以页为单位）
    static const size_t SPAN_PAGES = 8;

    void *Memory_Pool::CentralCache::fetchRange(size_t index)
    {
        // 索引检查，当索引≥FREE_LIST_SIZE时，说明申请内存过大，直接向系统申请
        if (index >= FREE_LIST_SIZE)
        {
            return nullptr;
        }

        // 自旋锁保护，检查并获取锁
        // test_and_set: 将std::atomic_flag的状态设置为true，并返回之前的状态；
        // 在这里就是进行加锁操作，若std::atomic_flag的状态为true，表示其他线程获取了锁，这里返回也为true，则进入循环，执行yield；若std::atomic_flag的状态为false，则设置为true后，返回false，表示当前进程进行了加锁操作
        while (locks_[index].test_and_set(std::memory_order_acquire))
        {
            // 添加线程让步，避免忙等待，避免过度消耗
            std::this_thread::yield();
            // 提示当前线程放弃 CPU 时间片，让操作系统调度其他线程运行
        }

        void *result = nullptr;
        try
        {
            // 尝试从中心缓存的现有空闲链表中获取内存块
            result = centralFreeList_[index].load(std::memory_order_relaxed);

            if (!result)
            {
                // 若中心缓存为空，从页缓存批量获取新的内存块
                size_t size = (index + 1) * ALIGNMENT;
                result = fetchFromPageCache(size);

                if (!result)
                {
                    // 若页缓存也为空，则解锁并返回空
                    locks_[index].clear(std::memory_order_release);
                    return nullptr;
                }

                // 将获取的内存块切分成小块，并构建链表放入中心缓存空闲链表中
                // !!这里将result转为char*是为了后续的算术运算，void*不支持算术运算操作(如void* + 1)，而char的大小固定为1字节，char*进行算术运算是以字节为单位进行的，若使用int*等类型，会以4字节为单位进行算术运算
                char *start = static_cast<char *>(result);
                // 计算根据当前块大小，需要分为几个内存块
                size_t blockNum = (SPAN_PAGES * PageCache::PAGE_SIZE) / size;

                // 确保至少有两个块才能构建链表
                if (blockNum > 1)
                {
                    // 将各个块连接起来
                    for (size_t i = 1; i < blockNum; ++i)
                    {
                        void *current = start + (i - 1) * size;

                        void *next = start + i * size;
                        *reinterpret_cast<void **>(current) = next;
                    }
                    // 最后一个节点next置为空
                    *reinterpret_cast<void **>(start + (blockNum - 1) * size) = nullptr;

                    // 保存result的下一个节点
                    void *next = *reinterpret_cast<void **>(result);
                    // 将result与链表断开
                    *reinterpret_cast<void **>(result) = nullptr;
                    // 更新中心缓存
                    centralFreeList_[index].store(next, std::memory_order_release);
                }
            }
            else
            {
                // 中心缓存不为空
                // 保存result的下一个节点
                void *next = *reinterpret_cast<void **>(result);
                // 将result与链表断开
                *reinterpret_cast<void **>(result) = nullptr;

                // 更新中心缓存
                centralFreeList_[index].store(next, std::memory_order_release);
            }
        }
        catch (...)
        {
            locks_[index].clear(std::memory_order_release);
            throw;
        }

        // 释放锁
        locks_[index].clear(std::memory_order_release);
        return result;
    }

    void CentralCache::returnRange(void *start, size_t size, size_t index)
    {
        // 当索引大于FREE_LIST_SIZE时，说明内存过大，直接向系统归还
        if (!start || index >= FREE_LIST_SIZE)
        {
            return;
        }

        while (locks_[index].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        try
        {
            // 尝试将归还的内存块插入中心缓存空闲链表
            void *current = centralFreeList_[index].load(std::memory_order_relaxed);
            *reinterpret_cast<void **>(start) = current;
            centralFreeList_[index].store(start, std::memory_order_release);
        }
        catch (...)
        {
            locks_[index].clear(std::memory_order_release);
            throw;
        }

        locks_[index].clear(std::memory_order_release);
    }

    void *CentralCache::fetchFromPageCache(size_t size)
    {
        // 计算实际需要的页数
        size_t numPages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;

        // 根据大小决定分配策略
        if (size <= SPAN_PAGES * PageCache::PAGE_SIZE)
        {
            // 小于等于32KB的请求，是用固定8页
            return PageCache::getInstance().allocateSpan(SPAN_PAGES);
        }
        else
        {
            // 大于32KB的请求，按实际需求分配
            return PageCache::getInstance().allocateSpan(numPages);
        }
    }
} // namespace Memory_Pool
