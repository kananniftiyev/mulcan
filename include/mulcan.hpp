#pragma once

#include "mulcan_errors.hpp"
#include "mulcan_infos.hpp"
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

	struct Vertex {
		glm::vec3 position;
		glm::vec3 color;
		// glm::vec2 texCoords;
	};


	// Init Functions
	MulcanResult initialize(GLFWwindow*& window);
	MulcanResult initializeCommands();
	MulcanResult initializeRenderPass();
	MulcanResult initializeFrameBuffer();

	// Render Functions
	void beginFrame();
	void endFrame();

	// Settigns functions
	void setVsync(bool value);
	void setFrameInFlight(FrameInFlight value);
	void setImgui(bool value);

	// Helper funcs

	VkPipelineLayout buildPipelineLayout(VkPushConstantRange range, uint32_t range_count, uint32_t layout_count, VkDescriptorSetLayout layout);
	VkPipeline buildPipeline(VkPipelineLayout& layout, VkRenderPass& pass);


	void cleanup();
}