#pragma once

#include "mulcan_errors.hpp"
#include "mulcan_infos.hpp"
#include "utils/shaders.hpp"
#include <array>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <VkBootstrap.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

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

	struct ImmediateSubmitData {
		VkFence fence;
		VkCommandPool pool;
		VkCommandBuffer cmd;
	};

	struct Vertex {
		glm::vec3 position;
		glm::vec3 color;
		// glm::vec2 texCoords;
	};

	// TODO: Rename
	struct AllocatedBuffer {
		VkBuffer buffer;
		VmaAllocation allocation;
	};

	struct TransferBuffer {
		VkBuffer src, dst;
		size_t buffer_size;
	};

	extern VmaAllocator g_vma_allocator;


	// TODO: one func for all.
	// Init Functions
	MulcanResult initialize(GLFWwindow*& window);
	MulcanResult initializeCommands();
	MulcanResult initializeRenderPass();
	MulcanResult initializeFrameBuffer();
	MulcanResult initializeTransferBuffer();

	// Render Functions
	void transferBufferCommand(TransferBuffer& buffer); // TODO: Make that make all commands once instead of many submits.
	void beginFrame();
	void endFrame();

	// Settigns functions
	void setVsync(bool value);
	void setFrameInFlight(FrameInFlight value);
	void setImgui(bool value);

	// Helper funcs

	VkPipelineLayout buildPipelineLayout(VkPushConstantRange range, uint32_t range_count, uint32_t layout_count, VkDescriptorSetLayout layout);
	VkPipeline buildPipeline(VkPipelineLayout& layout, VkRenderPass& pass, VkShaderModule vertex, VkShaderModule frag);

	// TODO: Remove template.
	template <typename T>
	TransferBuffer createTransferBuffer(std::vector<T> data, VkBufferUsageFlags flag) {

		const auto size = sizeof(T) * data.size();

		// CPU side
		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo vma_alloc_info{};
		vma_alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		AllocatedBuffer staging_buffer;

		CHECK_VK_LOG(vmaCreateBuffer(Mulcan::g_vma_allocator, &buffer_info, &vma_alloc_info, &staging_buffer.buffer, &staging_buffer.allocation, nullptr), "Could not create transfer Buffer");

		void* s_data;
		vmaMapMemory(Mulcan::g_vma_allocator, staging_buffer.allocation, &s_data);

		memcpy(s_data, data.data(), size);

		vmaUnmapMemory(Mulcan::g_vma_allocator, staging_buffer.allocation);

		// GPU side
		VkBufferCreateInfo gpu_buffer_info{};
		gpu_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		gpu_buffer_info.size = size;
		gpu_buffer_info.usage = flag | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vma_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		AllocatedBuffer gpu_buffer;

		CHECK_VK_LOG(vmaCreateBuffer(Mulcan::g_vma_allocator, &gpu_buffer_info, &vma_alloc_info, &gpu_buffer.buffer, &gpu_buffer.allocation, nullptr), "Could not create gpu buffer");

		return { staging_buffer, gpu_buffer, size };
	}


	void cleanup();
}