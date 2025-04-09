#pragma once

#include "mulcan_infos.hpp"
#include <array>
#include <fstream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <VkBootstrap.h>
#ifdef _WIN32
#include <vma/vk_mem_alloc.h>
#endif // _WIN32
#ifdef __linux__
#include <vk_mem_alloc.h>
#endif // _UNIX
#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>
#include <fstream>

#define CHECK_VK_LOG(res)                                                      \
	if (res != VK_SUCCESS)                                                     \
	{                                                                          \
		spdlog::error("Vulkan error: {} | File: {} | Line: {} | Function: {}", \
					  #res, __FILE__, __LINE__, __func__);                     \
		std::cout << "Hello";                                                  \
		abort();                                                               \
	}

namespace Mulcan
{
	enum class FrameInFlight
	{
		DOUBLE_BUFFERING,
		TRIPLE_BUFFERING,
	};

	struct FrameData
	{
		VkFence render_fence;
		VkSemaphore swapchain_semaphore, render_semaphore;
		VkCommandBuffer render_cmd;
		VkCommandPool render_pool;
	};

	struct ImmediateSubmitData
	{
		VkFence fence;
		VkCommandPool pool;
		VkCommandBuffer cmd;
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
		// glm::vec2 texCoords;
	};

	// TODO: Rename
	struct AllocatedBuffer
	{
		VkBuffer buffer;
		VmaAllocation allocation;
	};

	struct AllocatedImage
	{
		VkImage image;
		VmaAllocation allocation;
	};

	struct TransferBuffer
	{
		VkBuffer src, dst;
		size_t buffer_size;
	};

	struct NewPipelineDescription
	{
		VkShaderModule vertex_shader;
		VkShaderModule fragment_shader;

		VkVertexInputBindingDescription binding_description;
		std::array<VkVertexInputAttributeDescription, 2> input_attributes;

		VkRenderPass renderpass;
		VkPipelineLayout pipeline_layout;
	};

	extern VmaAllocator g_vma_allocator;

	// Init Functions
	void initialize(GLFWwindow *&window);

	// Render Functions
	void runTransferBufferCommand();
	void beginFrame();
	void endFrame();

	// Settigns functions
	void setVsync(bool value);
	void setImgui(bool value);

	// Helper funcs

	VkPipelineLayout buildPipelineLayout(const VkPushConstantRange &range, uint32_t range_count, uint32_t layout_count, const VkDescriptorSetLayout &layout);
	VkPipeline buildPipeline(const Mulcan::NewPipelineDescription &new_pipeline_data); // TODO: Pipeline desc data
	bool addTransferBuffer(const Mulcan::TransferBuffer &transfer_buffer);

	// TODO: Remove template.
	template <typename T>
	VkBuffer createTransferBuffer(std::vector<T> data, VkBufferUsageFlags flag)
	{

		const auto size = sizeof(T) * data.size();

		// CPU side
		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vma_alloc_info{};
		vma_alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		vma_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		AllocatedBuffer staging_buffer;
		CHECK_VK_LOG(vmaCreateBuffer(Mulcan::g_vma_allocator, &buffer_info, &vma_alloc_info, &staging_buffer.buffer, &staging_buffer.allocation, nullptr));

		void *s_data;
		vmaMapMemory(Mulcan::g_vma_allocator, staging_buffer.allocation, &s_data);

		memcpy(s_data, data.data(), size);

		vmaUnmapMemory(Mulcan::g_vma_allocator, staging_buffer.allocation);

		// GPU side
		VkBufferCreateInfo gpu_buffer_info{};
		gpu_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		gpu_buffer_info.size = size;
		gpu_buffer_info.usage = flag | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vma_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		vma_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		AllocatedBuffer gpu_buffer;

		CHECK_VK_LOG(vmaCreateBuffer(Mulcan::g_vma_allocator, &gpu_buffer_info, &vma_alloc_info, &gpu_buffer.buffer, &gpu_buffer.allocation, nullptr));

		TransferBuffer tb{};
		tb.src = staging_buffer.buffer;
		tb.dst = gpu_buffer.buffer;
		tb.buffer_size = size;

		auto res_add = addTransferBuffer(tb);
		if (!res_add)
		{
			spdlog::error("Could not add to transfer buffer");
			abort();
		}

		return gpu_buffer.buffer;
	}

	// Getter funcs
	VkCommandBuffer getCurrCommand();
	VkRenderPass getMainPass();

	bool loadShaderModule(const char *filePath, VkShaderModule *out_shader_module);

	void shutdown();
}