#pragma once

#include <vulkan/vulkan.h>

namespace MulcanInfos
{
    VkCommandPoolCreateInfo createCommandPoolInfo(uint32_t queue_family_index);
    VkCommandBufferAllocateInfo createCommandBufferAllocateInfo(VkCommandPool &pool, uint32_t buffer_count);
    VkFenceCreateInfo createFenceInfo();
    VkSemaphoreCreateInfo createSemaphoreInfo();
} // namespace MulcanInfos
