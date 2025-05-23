#pragma once

#include "mulcan_infos.hpp"
#include <array>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "mulkan_macros.hpp"
#include "pipeline.hpp"
#include "descriptor.hpp"
#include "camera.hpp"
#include "common_types.hpp"
#include <entt/entt.hpp>
#include "components.hpp"
#include "buffer_manager.hpp"

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

	struct NewPipelineDescription
	{
		VkShaderModule vertex_shader;
		VkShaderModule fragment_shader;

		VkVertexInputBindingDescription binding_description;
		std::array<VkVertexInputAttributeDescription, 4> input_attributes;

		VkRenderPass renderpass;
		VkPipelineLayout pipeline_layout;
	};

	struct MeshPushConstants
	{
		glm::vec4 data;
		glm::mat4 render_matrix;
	};

	extern VmaAllocator gVmaAllocator;

	// Init Functions
	void initialize(SDL_Window *&pWindow, uint32_t pWidth, uint32_t pHeight);

	// Render Functions
	void beginFrame();
	void endFrame();

	// Systems
	void renderWorldSystem();
	// TODO: void RenderDebugUISystem();
	// TODO: void RenderPostProcessSystem();
	// TODO: void RunAllSystems();

	// Objects
	bool spawnSampleCube();
	bool spawnCustomModel(const char *pFilePath);

	void removeModel(std::string name);

	glm::mat4 createModelMatrix(const glm::vec3 &pPos, const glm::vec3 &pRotation, const glm::vec3 &pScale);

	// Settigns functions
	void setVsync(bool pValue);
	void setImgui(bool pValue);

	// Getter funcs
	VkCommandBuffer getCurrCommand();
	VkRenderPass getMainPass();
	VkDevice &getDevice();
	VmaAllocator &getAllocator();

	void recreateSwapchain(uint32_t pWidth, uint32_t pHeight);

	void shutdown();
}