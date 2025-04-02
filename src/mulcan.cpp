#define VMA_IMPLEMENTATION
#include "mulcan.hpp"

// TODO: recreate the swapchain func.
// TODO: better naming
namespace Mulcan
{
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
	VkFormat g_depth_format;
	VkExtent2D g_window_extend{ .width = 800, .height = 600 };
	bool g_vsync = true;
	bool g_imgui = false;

#ifdef TRIBLE_BUFFER
	constexpr size_t FRAME_OVERLAP = 3;
#else
	constexpr size_t FRAME_OVERLAP = 2;
#endif // TRIBLE_BUFFER


	VkRenderPass main_pass;
	ImmediateSubmitData buffer_transfer;

	uint32_t framecount = 0;
	uint32_t g_swapchain_image_index;

	std::array<FrameData, FRAME_OVERLAP> frames;
	std::vector<VkFramebuffer> g_main_framebuffers;

	FrameData& getCurrFrame() { return frames[framecount % FRAME_OVERLAP]; }

	std::queue<TransferBuffer> g_transfer_buffers;

	AllocatedImage g_depth_image;
	VkImageView g_depth_image_view;

}

namespace
{
	Mulcan::MulcanResult initializeVulkan(GLFWwindow*& window)
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
		Mulcan::g_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
		Mulcan::g_queue_family_index = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

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

	Mulcan::MulcanResult initializeDepthImages() {
		Mulcan::g_depth_format = VK_FORMAT_D32_SFLOAT;

		VkExtent3D depth_extend{};
		depth_extend.depth = 1.0f;
		depth_extend.width = Mulcan::g_window_extend.width;
		depth_extend.height = Mulcan::g_window_extend.height;

		VkImageCreateInfo depth_image_info{};
		depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depth_image_info.imageType = VK_IMAGE_TYPE_2D;
		depth_image_info.extent = depth_extend;
		depth_image_info.format = Mulcan::g_depth_format;
		depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VmaAllocationCreateInfo depth_alloc_info{};
		depth_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		depth_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		CHECK_VK(vmaCreateImage(Mulcan::g_vma_allocator, &depth_image_info, &depth_alloc_info, &Mulcan::g_depth_image.image, &Mulcan::g_depth_image.allocation, nullptr));

		VkImageViewCreateInfo depth_image_view_info{};
		depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depth_image_view_info.image = Mulcan::g_depth_image.image;
		depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depth_image_view_info.format = Mulcan::g_depth_format;
		depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depth_image_view_info.subresourceRange.baseMipLevel = 0;
		depth_image_view_info.subresourceRange.levelCount = 1;
		depth_image_view_info.subresourceRange.baseArrayLayer = 0;
		depth_image_view_info.subresourceRange.layerCount = 1;

		CHECK_VK(vkCreateImageView(Mulcan::g_device, &depth_image_view_info, nullptr, &Mulcan::g_depth_image_view));
	}

	Mulcan::MulcanResult initializeCommands()
	{
		auto fence_info = MulcanInfos::createFenceInfo();
		auto semaphore_info = MulcanInfos::createSemaphoreInfo();

		for (size_t i = 0; i < Mulcan::FRAME_OVERLAP; i++)
		{
			auto command_pool_info = MulcanInfos::createCommandPoolInfo(Mulcan::g_queue_family_index);
			CHECK_VK(vkCreateCommandPool(Mulcan::g_device, &command_pool_info, nullptr, &Mulcan::frames[i].render_pool), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);

			auto command_buffer_info = MulcanInfos::createCommandBufferAllocateInfo(Mulcan::frames[i].render_pool, 1);
			CHECK_VK(vkAllocateCommandBuffers(Mulcan::g_device, &command_buffer_info, &Mulcan::frames[i].render_cmd), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);

			CHECK_VK(vkCreateFence(Mulcan::g_device, &fence_info, nullptr, &Mulcan::frames[i].render_fence), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);

			CHECK_VK(vkCreateSemaphore(Mulcan::g_device, &semaphore_info, nullptr, &Mulcan::frames[i].swapchain_semaphore), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);
			CHECK_VK(vkCreateSemaphore(Mulcan::g_device, &semaphore_info, nullptr, &Mulcan::frames[i].render_semaphore), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);
		}

		return Mulcan::MulcanResult::M_SUCCESS;
	}

	// TODO: depth attachment
	// TODO: abstract renderpass creation.
	Mulcan::MulcanResult initializeRenderPass()
	{
		VkAttachmentDescription color_attachment{
			.format = VK_FORMAT_B8G8R8A8_UNORM,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkAttachmentDescription depth_attachment{
			.format = Mulcan::g_depth_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};

		VkAttachmentReference color_attachment_refence{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkAttachmentReference depth_attachment_reference{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 2,
			.pColorAttachments = &color_attachment_refence,
			.pDepthStencilAttachment = &depth_attachment_reference };

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT };

		VkSubpassDependency depth_dependency = {};
		depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depth_dependency.dstSubpass = 0;
		depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depth_dependency.srcAccessMask = 0;
		depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachmenets;
		attachmenets[0] = color_attachment;
		attachmenets[1] = depth_attachment;

		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0] = dependency;
		dependencies[1] = depth_dependency;

		VkRenderPassCreateInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = dependencies.size(),
			.pAttachments = &attachmenets[0],
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = dependencies.size(),
			.pDependencies = &dependencies[0],
		};

		CHECK_VK(vkCreateRenderPass(Mulcan::g_device, &render_pass_info, nullptr, &Mulcan::main_pass), Mulcan::MulcanResult::M_RENDERPASS_ERROR);
		return Mulcan::MulcanResult::M_SUCCESS;
	}

	Mulcan::MulcanResult initializeFrameBuffer()
	{
		VkFramebufferCreateInfo fb_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = Mulcan::main_pass,
			.width = Mulcan::g_window_extend.width,
			.height = Mulcan::g_window_extend.height,
			.layers = 1 };

		Mulcan::g_main_framebuffers.resize(Mulcan::g_swapchain_images.size());
		for (int i = 0; i < Mulcan::g_swapchain_images.size(); i++)
		{
			std::array<VkImageView, 2> attachments{};

			attachments[0] = Mulcan::g_swapchain_image_views[i];
			attachments[1] = Mulcan::g_depth_image_view;

			fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			fb_info.pAttachments = attachments.data();

			CHECK_VK(vkCreateFramebuffer(Mulcan::g_device, &fb_info, nullptr, &Mulcan::g_main_framebuffers[i]), Mulcan::MulcanResult::M_FRAMEBUFFER_INIT_ERROR);
		}

		return Mulcan::MulcanResult::M_SUCCESS;
	}

	Mulcan::MulcanResult initializeTransferBuffer()
	{
		auto fence_info = MulcanInfos::createFenceInfo();
		CHECK_VK(vkCreateFence(Mulcan::g_device, &fence_info, nullptr, &Mulcan::buffer_transfer.fence), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);
		auto command_pool_info = MulcanInfos::createCommandPoolInfo(Mulcan::g_queue_family_index);
		CHECK_VK(vkCreateCommandPool(Mulcan::g_device, &command_pool_info, nullptr, &Mulcan::buffer_transfer.pool), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);

		auto command_buffer_info = MulcanInfos::createCommandBufferAllocateInfo(Mulcan::buffer_transfer.pool, 1);
		CHECK_VK(vkAllocateCommandBuffers(Mulcan::g_device, &command_buffer_info, &Mulcan::buffer_transfer.cmd), Mulcan::MulcanResult::M_COMMAND_INIT_ERROR);

		return Mulcan::MulcanResult::M_SUCCESS;
	}

}

Mulcan::MulcanResult Mulcan::initialize(GLFWwindow*& window)
{
	initializeVulkan(window);
	// TODO: Macro for no Depth;
	initializeDepthImages();
	initializeCommands();
	initializeRenderPass();
	initializeFrameBuffer();
	initializeTransferBuffer();
}

// TODO: Better way to get our dst buffer.
void Mulcan::runTransferBufferCommand()
{
	auto cmd = Mulcan::buffer_transfer.cmd;

	auto cmd_info = MulcanInfos::createCommandBufferBeginInfo(0);

	CHECK_VK_LOG(vkBeginCommandBuffer(cmd, &cmd_info), "Could not start transfer command");

	VkBufferCopy copy;
	copy.dstOffset = 0;
	copy.srcOffset = 0;

	while (!Mulcan::g_transfer_buffers.empty()) {
		copy.size = Mulcan::g_transfer_buffers.front().buffer_size;
		vkCmdCopyBuffer(cmd, Mulcan::g_transfer_buffers.front().src, Mulcan::g_transfer_buffers.front().dst, 1, &copy);
		Mulcan::g_transfer_buffers.pop();
	}

	CHECK_VK_LOG(vkEndCommandBuffer(cmd), "Could not end command buffer");

	vkResetFences(Mulcan::g_device, 1, &Mulcan::buffer_transfer.fence);

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.pNext = nullptr;

	info.waitSemaphoreCount = 0;
	info.pWaitSemaphores = nullptr;
	info.pWaitDstStageMask = nullptr;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmd;
	info.signalSemaphoreCount = 0;
	info.pSignalSemaphores = nullptr;

	CHECK_VK_LOG(vkQueueSubmit(Mulcan::g_queue, 1, &info, Mulcan::buffer_transfer.fence), "Could not submit transfer command");

	vkWaitForFences(Mulcan::g_device, 1, &Mulcan::buffer_transfer.fence, true, UINT32_MAX);

	vkResetCommandPool(Mulcan::g_device, Mulcan::buffer_transfer.pool, 0);
}

// TODO: remove allocations.
void Mulcan::beginFrame()
{
	CHECK_VK_LOG(vkWaitForFences(Mulcan::g_device, 1, &Mulcan::getCurrFrame().render_fence, true, UINT64_MAX), "Wait fence error.");
	CHECK_VK_LOG(vkResetFences(Mulcan::g_device, 1, &Mulcan::getCurrFrame().render_fence), "Reset Fence error");

	CHECK_VK_LOG(vkAcquireNextImageKHR(Mulcan::g_device, Mulcan::g_swapchain, UINT64_MAX, Mulcan::getCurrFrame().swapchain_semaphore, nullptr, &Mulcan::g_swapchain_image_index), "Acquire swapchain image error");

	CHECK_VK_LOG(vkResetCommandBuffer(Mulcan::getCurrFrame().render_cmd, 0), "Could not reset command buffer");

	auto render_cmd_info = MulcanInfos::createCommandBufferBeginInfo(0);

	VkClearValue clear_value[2]{};

	clear_value[0].color = { {50 / 255.0f, 60 / 255.0f, 68 / 255.0f, 1.0f} };
	clear_value[1].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo main_renderpass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = Mulcan::main_pass,
		.framebuffer = Mulcan::g_main_framebuffers[g_swapchain_image_index],
		.renderArea = {
			.offset = {0, 0},
			.extent = Mulcan::g_window_extend},
		.clearValueCount = 2,
		.pClearValues = &clear_value[0] };

	vkBeginCommandBuffer(Mulcan::getCurrFrame().render_cmd, &render_cmd_info);

	vkCmdBeginRenderPass(Mulcan::getCurrFrame().render_cmd, &main_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{
		.width = static_cast<float>(Mulcan::g_window_extend.width),
		.height = static_cast<float>(Mulcan::g_window_extend.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor{
		.offset = {0, 0},
		.extent = Mulcan::g_window_extend };

	vkCmdSetViewport(Mulcan::getCurrFrame().render_cmd, 0, 1, &viewport);
	vkCmdSetScissor(Mulcan::getCurrFrame().render_cmd, 0, 1, &scissor);
}

// TODO: Refactor to infos.
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
		.pSignalSemaphores = &Mulcan::getCurrFrame().render_semaphore };

	CHECK_VK_LOG(vkQueueSubmit(Mulcan::g_queue, 1, &submit_info, Mulcan::getCurrFrame().render_fence), "Could not submit command buffer");

	VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &Mulcan::getCurrFrame().render_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &Mulcan::g_swapchain,
		.pImageIndices = &Mulcan::g_swapchain_image_index };

	CHECK_VK_LOG(vkQueuePresentKHR(Mulcan::g_queue, &present_info), "Could not Present.");

	Mulcan::framecount++;
}

void Mulcan::setVsync(bool value)
{
	Mulcan::g_vsync = value;
}

void Mulcan::setImgui(bool value)
{
	Mulcan::g_imgui = value;
}

[[nodiscard]]
VkPipelineLayout Mulcan::buildPipelineLayout(const VkPushConstantRange& range, uint32_t range_count, uint32_t layout_count, const VkDescriptorSetLayout& layout)
{
	VkPipelineLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pushConstantRangeCount = range_count;
	info.pPushConstantRanges = &range;
	info.setLayoutCount = layout_count;
	info.pSetLayouts = &layout;

	VkPipelineLayout pipeline_layout;
	CHECK_VK_LOG(vkCreatePipelineLayout(Mulcan::g_device, &info, nullptr, &pipeline_layout), "Could not create pipeline layout.");

	return pipeline_layout;
}

// TODO: Caching support.
// TODO: Seperate shader stages.
[[nodiscard]]
VkPipeline Mulcan::buildPipeline(const Mulcan::NewPipelineData& new_pipeline_data)
{
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

	shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stages[0].module = new_pipeline_data.vertex_shader;
	shader_stages[0].pName = "main";
	shader_stages[0].pNext = nullptr;
	shader_stages[0].flags = 0;
	shader_stages[0].pSpecializationInfo = nullptr;

	shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages[1].module = new_pipeline_data.fragment_shader;
	shader_stages[1].pName = "main";
	shader_stages[1].pNext = nullptr;
	shader_stages[1].flags = 0;
	shader_stages[1].pSpecializationInfo = nullptr;

	auto pipeline_vertex_state = MulcanInfos::createPipelineVertexInputState(new_pipeline_data.binding_description, new_pipeline_data.input_attributes);

	auto input_assembly_state = MulcanInfos::createInputAssemblyStateInfo();

	VkPipelineViewportStateCreateInfo vp_info{};
	vp_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp_info.scissorCount = 1;
	vp_info.viewportCount = 1;

	auto raster_state_info = MulcanInfos::createRasterizationStateInfo();

	auto multisample_state_info = MulcanInfos::createMultisampleStateInfo(VK_FALSE, VK_SAMPLE_COUNT_1_BIT);

	std::vector<VkDynamicState> dynamic_states_enables;
	dynamic_states_enables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamic_states_enables.push_back(VK_DYNAMIC_STATE_SCISSOR);

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pDynamicStates = dynamic_states_enables.data();
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states_enables.size());

	VkPipelineColorBlendAttachmentState blend_attachment_state{};
	blend_attachment_state.colorWriteMask = 0xf;
	blend_attachment_state.blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo color_blend_state{};
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &blend_attachment_state;

	VkGraphicsPipelineCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount = static_cast<uint32_t>(shader_stages.size());
	info.pStages = shader_stages.data();
	info.pVertexInputState = &pipeline_vertex_state;
	info.pInputAssemblyState = &input_assembly_state;
	info.pViewportState = &vp_info;
	info.pRasterizationState = &raster_state_info;
	info.pMultisampleState = &multisample_state_info;
	info.pDynamicState = &dynamic_state;
	info.layout = new_pipeline_data.pipeline_layout;
	info.renderPass = new_pipeline_data.renderpass;
	info.pNext = nullptr;
	info.pColorBlendState = &color_blend_state;

	VkPipeline pipeline;
	CHECK_VK_LOG(vkCreateGraphicsPipelines(Mulcan::g_device, nullptr, 1, &info, nullptr, &pipeline), "Could not create pipeline.");

	vkDestroyShaderModule(Mulcan::g_device, new_pipeline_data.vertex_shader, nullptr);
	vkDestroyShaderModule(Mulcan::g_device, new_pipeline_data.fragment_shader, nullptr);

	return pipeline;
}

Mulcan::MulcanResult Mulcan::addTransferBuffer(const Mulcan::TransferBuffer& transfer_buffer)
{
	if (transfer_buffer.buffer_size == 0 || transfer_buffer.src == VK_NULL_HANDLE || transfer_buffer.dst == VK_NULL_HANDLE)
	{
		return Mulcan::MulcanResult::M_EMPTY_VARS_OR_BUFFERS;
	}
	Mulcan::g_transfer_buffers.push(transfer_buffer);

	return Mulcan::MulcanResult::M_SUCCESS;
}

VkCommandBuffer Mulcan::getCurrCommand()
{
	return Mulcan::getCurrFrame().render_cmd;
}

VkRenderPass Mulcan::getMainPass()
{
	return Mulcan::main_pass;
}

bool Mulcan::loadShaderModule(const char* filePath, VkShaderModule* out_shader_module)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	size_t fileSize = (size_t)file.tellg();

	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);

	file.read((char*)buffer.data(), fileSize);

	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Mulcan::g_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return false;
	}
	*out_shader_module = shaderModule;
	return true;
}

// TODO: deletion queue
void Mulcan::shutdown()
{

	for (auto& framebuffer : Mulcan::g_main_framebuffers)
	{
		vkDestroyFramebuffer(Mulcan::g_device, framebuffer, nullptr);
	}

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
