#include "../include/MemoryPool.h"

namespace Memory_Pool
{

    MemoryPool::MemoryPool(size_t BlockSize)
        : BlockSize_(BlockSize), SlotSize_(0), firstBlock_(nullptr), curSlot_(nullptr), freeLists_(nullptr), lastSlot_(nullptr)
    {
    }

    MemoryPool::~MemoryPool()
    {
        // 把连续的block删除
        Slot *cur = firstBlock_;
        while (cur)
        {
            Slot *next = cur->next;
            // 等同于 free(reinterpret_cast<void*>(firstBlock_));
            // 转化为 void指针，因为 void 类型不需要调用析构函数，只释放空间
            operator delete(reinterpret_cast<void *>(cur));
            cur = next;
        }
    }

    void MemoryPool::init(size_t size)
    {
        assert(size > 0);
        SlotSize_ = size;
        firstBlock_ = nullptr;
        curSlot_ = nullptr;
        freeLists_ = nullptr;
        lastSlot_ = nullptr;
    }

    void *MemoryPool::allocate()
    {
        // 优先使用空闲链表中的内存槽
        Slot *slot = popFreeList();
        if (slot != nullptr)
        {
            return slot;
        }

        Slot *temp;
        {
            std::lock_guard<std::mutex> lock(mutexForBlock_);
            if (curSlot_ >= lastSlot_)
            {
                // 当前内存块已无内存槽可用，开辟一块新的内存
                allocateNewBlock();
            }

            temp = curSlot_;
            // 这里不能直接 curSlot_ += SlotSize_ 因为curSlot_是Slot*类型，需要除以SlotSize_再加1
            curSlot_ += SlotSize_ / sizeof(Slot);
        }

        return temp;
    }

    void MemoryPool::deallocate(void *ptr)
    {
        if (!ptr)
        {
            return;
        }
        Slot *slot = reinterpret_cast<Slot *>(ptr);
        pushFreeList(slot);
    }

    void MemoryPool::allocateNewBlock()
    {
        std::cout << "申请一块内存块, SlotSize: " << SlotSize_ << std::endl;
        // 头插法插入新的内存块
        void *newBlock = operator new(BlockSize_);
        reinterpret_cast<Slot *>(newBlock)->next = firstBlock_;
        firstBlock_ = reinterpret_cast<Slot *>(newBlock);

        char *body = reinterpret_cast<char *>(newBlock) + sizeof(Slot *);
        size_t paddingSize = padPointer(body, SlotSize_); // 计算对齐需要填充的内存大小
        curSlot_ = reinterpret_cast<Slot *>(body + paddingSize);

        // 超过该标记位置，则说明该内存块已无内存槽可用，需向系统申请新的内存块
        lastSlot_ = reinterpret_cast<Slot *>(reinterpret_cast<size_t>(newBlock) + BlockSize_ - SlotSize_ + 1);

        freeLists_ = nullptr;
    }

    size_t MemoryPool::padPointer(char *p, size_t align)
    {
        // align是槽大小
        return (align - reinterpret_cast<size_t>(p)) % align;
    }

    bool MemoryPool::pushFreeList(Slot *slot)
    {
        while (true)
        {
            // 获取当前头结点
            Slot *oldHead = freeLists_.load(std::memory_order_relaxed);
            // 将新节点的 next 指向当前头结点
            slot->next.store(oldHead, std::memory_order_relaxed);

            // 尝试将新节点设置为头结点
            if (freeLists_.compare_exchange_weak(oldHead, slot, std::memory_order_release, std::memory_order_relaxed))
            {
                return true;
            }
            // 失败：说明另一个线程可能已经修改了 freeLists_
            // CAS 失败则重试
        }
    }

    Slot *MemoryPool::popFreeList()
    {
        while (true)
        {
            Slot *oldHead = freeLists_.load(std::memory_order_acquire);
            if (oldHead == nullptr)
            {
                return nullptr;
            }

            // 在访问 newHead 之前再次验证 oldHead 的有效性
            Slot *newHead = nullptr;
            try
            {
                newHead = oldHead->next.load(std::memory_order_relaxed);
            }
            catch (...)
            {
                continue;
            }

            // 尝试更新头结点
            // 原子性地尝试将 freeList_ 从 oldHead 更新为 newHead
            if (freeLists_.compare_exchange_weak(oldHead, newHead, std::memory_order_acquire, std::memory_order_relaxed))
            {
                return oldHead;
            }
            // 失败：说明另一个线程可能已经修改了 freeLists_
            // CAS 失败则重试
        }
    }

    void HashBucket::initMemoryPool()
    {
        for (int i = 0; i < MEMORY_POOL_NUM; ++i)
        {
            getMemoryPool(i).init((i + 1) * SLOT_BASE_SIZE);
        }
    }

    MemoryPool &HashBucket::getMemoryPool(int index)
    {
        static MemoryPool memoryPool[MEMORY_POOL_NUM];
        return memoryPool[index];
    }

    void *HashBucket::useMemory(size_t size)
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

    void HashBucket::freeMemory(void *ptr, size_t size)
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

} // namespace Memory_Pool
