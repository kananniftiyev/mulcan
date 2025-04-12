#pragma once

#ifdef _WIN32
#include <vma/vk_mem_alloc.h>
#endif // _WIN32
#ifdef __linux__
#include <vk_mem_alloc.h>
#endif // _UNIX
#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>
#include "mulkan_macros.hpp"
#include <unordered_map>
#include <vector>

namespace Mulcan
{
    struct AllocatedBuffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
    };

    class DescriptorManager
    {
    private:
        VmaAllocator &mAllocator;
        VkDevice &mDevice;
        std::unordered_map<int, VkDescriptorSetLayout> mDescriptorLayoutMap;
        std::unordered_map<int, VkDescriptorSet> mDescriptorSetMap;

        VkDescriptorPool mStaticObjectPool;
        VkDescriptorPool mDynamicObjectPool;

    public:
        DescriptorManager(VmaAllocator &pAllocator, VkDevice &pDevice);

        void CreateDescriptorPool();

        void CreateDescriptorLayout(int id, const std::vector<VkDescriptorSetLayoutBinding> &pBindings);
        void CreateDescriptorSet(int id, VkBuffer &pBuffer, size_t pSize);

        AllocatedBuffer CreateUniformBuffer(size_t pAllocSize);

        template <typename T>
        void Mulcan::DescriptorManager::UpdateBuffer(AllocatedBuffer *pAllocatedBuffer, T pData)
        {
            void *data;
            vmaMapMemory(this->mAllocator, pAllocatedBuffer->allocation, &data);

            memcpy(data, pData, sizeof(T));

            vmaUnmapMemory(this->mAllocator, pAllocatedBuffer->allocation);
        }

        void Cleanup();
    };

} // namespace Mulcan
