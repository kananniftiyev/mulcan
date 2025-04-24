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
#include "frameresource.hpp"

namespace Mulcan::Descriptor
{
    struct SingleDescriptorCtx
    {
        VkDescriptorSet set;
        VkDescriptorSetLayout layout;
        std::vector<AllocatedBuffer> buffers;
    };

    struct DoubleDescriptorCtx
    {
        std::array<VkDescriptorSet, 2> set;
        VkDescriptorSetLayout layout;
        std::array<AllocatedBuffer, 2> buffers;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
        VkBufferCreateInfo bufferCreateInfo;
        VkDescriptorPool pool;
        size_t allocSize;
    };

    void addBindingToSet(const VkDescriptorSetLayoutBinding &binding, DoubleDescriptorCtx *ctx);
    void addBufferToSet(size_t allocationSize, DoubleDescriptorCtx *ctx);
    void updateData(VmaAllocator &allocator, VmaAllocation allocation, const void *pData, size_t size);
    void buildSet(VkDevice &device, VmaAllocator &allocator, DoubleDescriptorCtx *ctx);

} // namespace Mulcan
