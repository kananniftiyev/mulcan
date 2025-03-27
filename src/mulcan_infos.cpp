#include "mulcan_infos.hpp"

[[nodiscard]]
VkCommandPoolCreateInfo MulcanInfos::createCommandPoolInfo(uint32_t queue_family_index)
{
	return VkCommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queue_family_index,
	};
}

[[nodiscard]]
VkCommandBufferAllocateInfo MulcanInfos::createCommandBufferAllocateInfo(VkCommandPool& pool, uint32_t buffer_count)
{
	return VkCommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = buffer_count,
	};
}

[[nodiscard]]
VkFenceCreateInfo MulcanInfos::createFenceInfo()
{
	return VkFenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT };
}

[[nodiscard]]
VkSemaphoreCreateInfo MulcanInfos::createSemaphoreInfo()
{
	return VkSemaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0 };
}

[[nodiscard]]
VkPipelineRasterizationStateCreateInfo MulcanInfos::createRasterizationStateInfo()
{
	VkPipelineRasterizationStateCreateInfo raster_state_info{};
	raster_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_state_info.polygonMode = VK_POLYGON_MODE_FILL;
	raster_state_info.cullMode = VK_CULL_MODE_FRONT_BIT;
	raster_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster_state_info.depthClampEnable = VK_FALSE;
	raster_state_info.rasterizerDiscardEnable = VK_FALSE;
	raster_state_info.depthBiasEnable = VK_FALSE;
	raster_state_info.lineWidth = 1.0f;

	return raster_state_info;
}

[[nodiscard]]
VkPipelineMultisampleStateCreateInfo MulcanInfos::createMultisampleStateInfo(VkBool32 sample_shading, VkSampleCountFlagBits sample_count)
{
	VkPipelineMultisampleStateCreateInfo multisample_state_info{};
	multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_info.rasterizationSamples = sample_count;
	multisample_state_info.sampleShadingEnable = sample_shading;

	return multisample_state_info;
}

[[nodiscard]]
VkPipelineInputAssemblyStateCreateInfo MulcanInfos::createInputAssemblyStateInfo()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;

	return input_assembly_state;
}

