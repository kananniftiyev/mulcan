#include "mulcan_infos.hpp"

VkCommandPoolCreateInfo MulcanInfos::createCommandPoolInfo(uint32_t queue_family_index)
{
    return VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .queueFamilyIndex = queue_family_index,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
}

VkCommandBufferAllocateInfo MulcanInfos::createCommandBufferAllocateInfo(VkCommandPool &pool, uint32_t buffer_count)
{
    return VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = buffer_count,
        .pNext = nullptr};
}

VkFenceCreateInfo MulcanInfos::createFenceInfo()
{
    return VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT};
}

VkSemaphoreCreateInfo MulcanInfos::createSemaphoreInfo()
{
    return VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0};
}
