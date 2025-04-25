#pragma once

#ifdef _WIN32
#include <vma/vk_mem_alloc.h>
#endif // _WIN32
#ifdef __linux__
#include <vk_mem_alloc.h>
#endif // _UNIX
#include <vulkan/vulkan.h>
#include "mulkan_macros.hpp"
#include <unordered_map>
#include <vector>
#include "common_types.hpp"

namespace Mulcan::Descriptor
{
    struct SingleBufferedDescriptorCtx
    {
        VkDescriptorSet set;
        VkDescriptorSetLayout layout;
        std::vector<AllocatedBuffer> buffers;
    };

    struct DoubleBufferedDescriptorCtx
    {
        std::array<VkDescriptorSet, 2> set;
        std::array<AllocatedBuffer, 2> buffers;
        VkDescriptorSetLayout layout;
    };

    struct DoubleBufferedDescriptorBuildCtx
    {
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
        VkBufferCreateInfo bufferCreateInfo;
        VkDescriptorPool pool;
        size_t allocSize;
    };

    void addBindingToSetDB(const VkDescriptorSetLayoutBinding &binding, DoubleBufferedDescriptorBuildCtx *ctx);
    void addBufferToSetDB(size_t allocationSize, DoubleBufferedDescriptorBuildCtx *ctx);
    void updateData(VmaAllocator &allocator, VmaAllocation allocation, const void *pData, size_t size);
    DoubleBufferedDescriptorCtx buildSetDB(VkDevice &device, VmaAllocator &allocator, DoubleBufferedDescriptorBuildCtx *ctx);

} // namespace Mulcan
