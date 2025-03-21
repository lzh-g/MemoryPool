#include "PageCache.h"

#include <cstring>
#include <sys/mman.h>

namespace Memory_Pool
{
    void *PageCache::allocateSpan(size_t numPages)
    {
        return nullptr;
    }

    void PageCache::deallocateSpan(void *ptr, size_t numPages)
    {
    }

    void *PageCache::systemAlloc(size_t numPages)
    {
        return nullptr;
    }

} // namespace Memory_Pool
