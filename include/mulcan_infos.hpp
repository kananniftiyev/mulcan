#pragma once

#include <array>
#include <vulkan/vulkan.h>

namespace MulcanInfos
{
	VkCommandPoolCreateInfo createCommandPoolInfo(uint32_t queue_family_index);
	VkCommandBufferAllocateInfo createCommandBufferAllocateInfo(VkCommandPool &pool, uint32_t buffer_count);
	VkFenceCreateInfo createFenceInfo();
	VkSemaphoreCreateInfo createSemaphoreInfo();
	VkPipelineRasterizationStateCreateInfo createRasterizationStateInfo();
	VkPipelineMultisampleStateCreateInfo createMultisampleStateInfo(VkBool32 sample_shading, VkSampleCountFlagBits sample_count);
	VkPipelineInputAssemblyStateCreateInfo createInputAssemblyStateInfo();
	VkCommandBufferBeginInfo createCommandBufferBeginInfo(VkCommandBufferUsageFlags flag);

	VkPipelineVertexInputStateCreateInfo createPipelineVertexInputState(const VkVertexInputBindingDescription &input_binding, const std::array<VkVertexInputAttributeDescription, 2> &input_attributes);
} // namespace MulcanInfos
