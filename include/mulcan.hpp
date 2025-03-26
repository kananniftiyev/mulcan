#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <vector>
#include "mulcan_errors.hpp"
#include <array>
#include "mulcan_infos.hpp"

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


	// Init Functions
	MulcanResult initialize(VkSurfaceKHR surface);
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

	void cleanup();
}