#pragma once

#include "mulcan_errors.hpp"
#include "mulcan_infos.hpp"
#include <array>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
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

	FrameData& getCurrFrame() { return frames[FRAME_OVERLAP % framecount]; }

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

	void cleanup();
}