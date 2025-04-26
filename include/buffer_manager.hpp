#pragma once

#include <queue>
#include <vector>
#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <vma/vk_mem_alloc.h>
#endif // _WIN32
#ifdef __linux__
#include <vk_mem_alloc.h>
#endif // _UNIX
#include "common_types.hpp"
#include "mulkan_macros.hpp"
#include "mulcan_infos.hpp"
#include "mulkan_macros.hpp"

struct TransferBuffer
{
    AllocatedBuffer src, dst;
    size_t buffer_size;
};

class BufferManager
{
private:
    std::queue<AllocatedBuffer> mDeletionBuffers;
    std::vector<TransferBuffer> mStagingBuffers;

    const VkDevice *mDevice;
    const VmaAllocator *mAllocator;
    std::uint32_t mBufferCount = 0;

public:
    BufferManager(const VkDevice *device, const VmaAllocator *allocator);

    AllocatedBuffer createCPUOnlyBuffer(const void *data, size_t allocSize, VkBufferUsageFlags pFlag, VkBufferUsageFlagBits usage);
    AllocatedBuffer createStagingBuffer(const void *data, size_t allocSize, VkBufferUsageFlags pFlag);
    AllocatedBuffer createCPUToGPUBuffer(const void *data, size_t allocSize, VkBufferUsageFlags pFlag, VkBufferUsageFlagBits usage);

    void releaseNow(VkBuffer &buffer);
    void releaseAllResources();

    std::uint32_t getBufferCount() const;
    std::vector<TransferBuffer> &getStaginBuffers();
};
