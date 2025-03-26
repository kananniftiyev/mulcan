#define VMA_IMPLEMENTATION
#include "mulcan.hpp"

namespace Mulcan {
	VkDevice g_device;
	VkInstance g_instance;
	VkPhysicalDevice g_physical_device;
	VkQueue g_queue;
	uint32_t g_queue_family_index;
	VkSurfaceKHR g_surface;
	VkSwapchainKHR g_swapchain;
	VkDebugUtilsMessengerEXT g_debug_messenger;
	std::vector<VkImage> g_swapchain_images;
	std::vector<VkImageView> g_swapchain_image_views;
	VmaAllocator g_vma_allocator;
	VkFormat g_swapchain_format;
	VkExtent2D g_window_extend{ .width = 800, .height = 600 };
	bool g_vsync = true;
	bool g_imgui = false;
	constexpr size_t FRAME_OVERLAP = 2;
	VkRenderPass main_pass;

	uint32_t framecount = 0;
	uint32_t swapchain_image_index;


	std::array<FrameData, FRAME_OVERLAP> frames;
	std::vector<VkFramebuffer> g_main_frame_buffers;

}

Mulcan::MulcanResult Mulcan::initialize(GLFWwindow*& window)
{
	vkb::InstanceBuilder instance_builder;
	auto inst_ret = instance_builder.request_validation_layers()
		.require_api_version(1, 2, 0)
		.use_default_debug_messenger()
		.set_app_name("Mulcan")
		.build();

	auto vkb_inst = inst_ret.value();

	Mulcan::g_instance = vkb_inst.instance;
	Mulcan::g_debug_messenger = vkb_inst.debug_messenger;

	CHECK_VK(glfwCreateWindowSurface(Mulcan::g_instance, window, nullptr, &Mulcan::g_surface), Mulcan::MulcanResult::M_UNKNOWN_ERROR);

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physical_device = selector
		.set_minimum_version(1, 2)
		.set_surface(Mulcan::g_surface)
		.select()
		.value();

	vkb::DeviceBuilder deviceBuilder{ physical_device };

	vkb::Device vkb_device = deviceBuilder.build().value();

	Mulcan::g_device = vkb_device.device;
	Mulcan::g_physical_device = physical_device.physical_device;

	vkb::SwapchainBuilder swapchainBuilder{ Mulcan::g_physical_device, Mulcan::g_device, Mulcan::g_surface };

	Mulcan::g_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkb_swapchain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{ .format = Mulcan::g_swapchain_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode((Mulcan::g_vsync) ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
		.set_desired_extent(800, 600)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	Mulcan::g_window_extend = vkb_swapchain.extent;
	Mulcan::g_swapchain = vkb_swapchain.swapchain;
	Mulcan::g_swapchain_images = vkb_swapchain.get_images().value();
	Mulcan::g_swapchain_image_views = vkb_swapchain.get_image_views().value();

	VmaAllocatorCreateInfo allocator_create_info = {};
	allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
	allocator_create_info.physicalDevice = Mulcan::g_physical_device;
	allocator_create_info.device = Mulcan::g_device;
	allocator_create_info.instance = Mulcan::g_instance;

	auto res = vmaCreateAllocator(&allocator_create_info, &Mulcan::g_vma_allocator);
	CHECK_VK(res, Mulcan::MulcanResult::M_VMA_ERROR);

	return Mulcan::MulcanResult::M_SUCCESS;
}

Mulcan::MulcanResult Mulcan::initializeCommands()
{
	auto fence_info = MulcanInfos::createFenceInfo();
	auto semaphore_info = MulcanInfos::createSemaphoreInfo();

	for (size_t i = 0; i < Mulcan::FRAME_OVERLAP; i++)
	{
		auto command_pool_info = MulcanInfos::createCommandPoolInfo(Mulcan::g_queue_family_index);
		CHECK_VK(vkCreateCommandPool(Mulcan::g_device, &command_pool_info, nullptr, &frames[i].render_pool), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);

		auto command_buffer_info = MulcanInfos::createCommandBufferAllocateInfo(frames[i].render_pool, 1);
		CHECK_VK(vkAllocateCommandBuffers(Mulcan::g_device, &command_buffer_info, &frames[i].render_cmd), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);

		CHECK_VK(vkCreateFence(Mulcan::g_device, &fence_info, nullptr, &frames[i].render_fence), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);

		CHECK_VK(vkCreateSemaphore(Mulcan::g_device, &semaphore_info, nullptr, &frames[i].swapchain_semaphore), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);
		CHECK_VK(vkCreateSemaphore(Mulcan::g_device, &semaphore_info, nullptr, &frames[i].render_semaphore), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);
	}

	return Mulcan::MulcanResult::M_SUCCESS;
}

Mulcan::MulcanResult Mulcan::initializeRenderPass()
{
	VkAttachmentDescription color_attachment{
		.format = Mulcan::g_swapchain_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentReference color_attachment_refence{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_refence };

	VkSubpassDependency dependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT };

	VkRenderPassCreateInfo render_pass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};

	CHECK_VK(vkCreateRenderPass(Mulcan::g_device, &render_pass_info, nullptr, &main_pass), Mulcan::MulcanResult::M_RENDERPASS_ERROR);
	return Mulcan::MulcanResult::M_SUCCESS;
}

Mulcan::MulcanResult Mulcan::initializeFrameBuffer()
{
	VkFramebufferCreateInfo fb_info{
	.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
	.pNext = nullptr,
	.flags = 0,
	.renderPass = main_pass,
	.width = Mulcan::g_window_extend.width,
	.height = Mulcan::g_window_extend.height,
	.layers = 1
	};

	Mulcan::g_main_frame_buffers.resize(Mulcan::g_swapchain_images.size());
	for (int i = 0; i < Mulcan::g_swapchain_images.size(); i++)
	{
		std::array<VkImageView, 1> attachments{};

		attachments[0] = Mulcan::g_swapchain_image_views[i];

		fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		fb_info.pAttachments = attachments.data();

		CHECK_VK(vkCreateFramebuffer(Mulcan::g_device, &fb_info, nullptr, &Mulcan::g_main_frame_buffers[i]), Mulcan::MulcanResult::M_FRAMEBUFFER_INIT_ERROR);
	}

	return Mulcan::MulcanResult::M_SUCCESS;
}

void Mulcan::beginFrame()
{
	CHECK_VK_LOG(vkWaitForFences(Mulcan::g_device, 1, &Mulcan::getCurrFrame().render_fence, true, UINT64_MAX), "Wait fence error.");
	CHECK_VK_LOG(vkResetFences(Mulcan::g_device, 1, &Mulcan::getCurrFrame().render_fence), "Reset Fence error");

	CHECK_VK_LOG(vkAcquireNextImageKHR(Mulcan::g_device, Mulcan::g_swapchain, UINT64_MAX, Mulcan::getCurrFrame().swapchain_semaphore, nullptr, &Mulcan::swapchain_image_index), "Acquire swapchain image error");

	CHECK_VK_LOG(vkResetCommandBuffer(Mulcan::getCurrFrame().render_cmd, 0), "Could not reset command buffer");

	VkCommandBufferBeginInfo render_cmd_info{
	.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	.pNext = nullptr,
	};

	VkClearValue clear_value[1]{};

	clear_value[0].color = { {0.3f, 0.5f, 0.7f, 1.0f} };

	VkRenderPassBeginInfo main_renderpass_info{
	.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	.renderPass = Mulcan::main_pass,
	.framebuffer = Mulcan::g_main_frame_buffers[swapchain_image_index],
	.renderArea = {
			.offset = {0, 0},
			.extent = Mulcan::g_window_extend
},
	.clearValueCount = 1,
	.pClearValues = &clear_value[0]
	};

	vkBeginCommandBuffer(Mulcan::getCurrFrame().render_cmd, &render_cmd_info);

	vkCmdBeginRenderPass(Mulcan::getCurrFrame().render_cmd, &main_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{
	.width = Mulcan::g_window_extend.width,
	.height = Mulcan::g_window_extend.height,
	.minDepth = 0.0f,
	.maxDepth = 1.0f,
	};

	VkRect2D scissor{
		.offset = {0, 0},
		.extent = Mulcan::g_window_extend
	};

	vkCmdSetViewport(Mulcan::getCurrFrame().render_cmd, 0, 1, &viewport);
	vkCmdSetScissor(Mulcan::getCurrFrame().render_cmd, 0, 1, &scissor);

}

void Mulcan::endFrame()
{

	vkCmdEndRenderPass(Mulcan::getCurrFrame().render_cmd);
	vkEndCommandBuffer(Mulcan::getCurrFrame().render_cmd);

	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;


	VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &Mulcan::getCurrFrame().swapchain_semaphore,
		.pWaitDstStageMask = &wait_stage_mask,
		.commandBufferCount = 1,
		.pCommandBuffers = &Mulcan::getCurrFrame().render_cmd,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &Mulcan::getCurrFrame().render_semaphore
	};

	CHECK_VK_LOG(vkQueueSubmit(Mulcan::g_queue, 1, &submit_info, Mulcan::getCurrFrame().render_fence), "Could not submit command buffer");

	VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &Mulcan::getCurrFrame().render_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &Mulcan::g_swapchain,
		.pImageIndices = &Mulcan::swapchain_image_index
	};

	CHECK_VK_LOG(vkQueuePresentKHR(Mulcan::g_queue, &present_info), "Could not Present.");
}

void Mulcan::setVsync(bool value)
{
	Mulcan::g_vsync = value;
}

void Mulcan::setFrameInFlight(FrameInFlight value)
{
}

void Mulcan::setImgui(bool value)
{
	Mulcan::g_imgui = value;
}

void Mulcan::cleanup()
{
	vkDestroyRenderPass(Mulcan::g_device, Mulcan::main_pass, nullptr);
	for (auto& frame : frames)
	{
		vkDestroyCommandPool(Mulcan::g_device, frame.render_pool, nullptr);
		vkDestroyFence(Mulcan::g_device, frame.render_fence, nullptr);
		vkDestroySemaphore(Mulcan::g_device, frame.render_semaphore, nullptr);
		vkDestroySemaphore(Mulcan::g_device, frame.swapchain_semaphore, nullptr);
	}
	for (auto& view : Mulcan::g_swapchain_image_views)
	{
		vkDestroyImageView(Mulcan::g_device, view, nullptr);
	}
	vmaDestroyAllocator(Mulcan::g_vma_allocator);
	vkDestroySwapchainKHR(Mulcan::g_device, Mulcan::g_swapchain, nullptr);
	vkDestroyDevice(Mulcan::g_device, nullptr);
	vkDestroySurfaceKHR(Mulcan::g_instance, Mulcan::g_surface, nullptr);
	vkb::destroy_debug_utils_messenger(Mulcan::g_instance, Mulcan::g_debug_messenger, nullptr);
	vkDestroyInstance(Mulcan::g_instance, nullptr);
}
