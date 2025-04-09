#define VMA_IMPLEMENTATION
#include "mulcan.hpp"

// TODO: recreate the swapchain func.
// TODO: better naming
namespace Mulcan
{
	namespace Settings
	{
		bool g_vsync = true;
		bool g_imgui = false;
		bool g_has_depth = true;
		VkExtent2D g_window_extend{.width = 800, .height = 600};
	}

	namespace VKContext
	{
		VkDevice g_device;
		VkInstance g_instance;
		VkPhysicalDevice g_physical_device;
		VkQueue g_queue;
		uint32_t g_queue_family_index;
		VkSurfaceKHR g_surface;
		VkSwapchainKHR g_swapchain;
		VkDebugUtilsMessengerEXT g_debug_messenger;
	}

	VmaAllocator g_vma_allocator;

	namespace Render
	{
		std::vector<VkImage> g_swapchain_images;
		std::vector<VkImageView> g_swapchain_image_views;
		VkFormat g_swapchain_format;
		VkFormat g_depth_format;
		VkRenderPass main_pass;
		std::array<FrameData, 2> frames;
		std::vector<VkFramebuffer> g_main_framebuffers;
		AllocatedImage g_depth_image;
		VkImageView g_depth_image_view;
		uint32_t framecount = 0;
		uint32_t g_swapchain_image_index;
	}

#ifdef TRIBLE_BUFFER
	constexpr size_t FRAME_OVERLAP = 3;
#else
	constexpr size_t FRAME_OVERLAP = 2;
#endif // TRIBLE_BUFFER

	ImmediateSubmitData buffer_transfer;
	std::queue<TransferBuffer> g_transfer_buffers;

	FrameData &getCurrFrame() { return Render::frames[Render::framecount % FRAME_OVERLAP]; }

}

namespace
{
	void initializeVulkan(GLFWwindow *&window)
	{
		vkb::InstanceBuilder instance_builder;
		auto inst_ret = instance_builder.request_validation_layers()
							.require_api_version(1, 2, 0)
							.use_default_debug_messenger()
							.set_app_name("Mulcan")
							.build();

		auto vkb_inst = inst_ret.value();

		Mulcan::VKContext::g_instance = vkb_inst.instance;
		Mulcan::VKContext::g_debug_messenger = vkb_inst.debug_messenger;

		CHECK_VK_LOG(glfwCreateWindowSurface(Mulcan::VKContext::g_instance, window, nullptr, &Mulcan::VKContext::g_surface));

		vkb::PhysicalDeviceSelector selector{vkb_inst};
		vkb::PhysicalDevice physical_device = selector
												  .set_minimum_version(1, 2)
												  .set_surface(Mulcan::VKContext::g_surface)
												  .select()
												  .value();

		vkb::DeviceBuilder deviceBuilder{physical_device};

		vkb::Device vkb_device = deviceBuilder.build().value();

		Mulcan::VKContext::g_device = vkb_device.device;
		Mulcan::VKContext::g_physical_device = physical_device.physical_device;

		vkb::SwapchainBuilder swapchainBuilder{Mulcan::VKContext::g_physical_device, Mulcan::VKContext::g_device, Mulcan::VKContext::g_surface};

		Mulcan::Render::g_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkb_swapchain = swapchainBuilder
										   .set_desired_format(VkSurfaceFormatKHR{.format = Mulcan::Render::g_swapchain_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
										   .set_desired_present_mode((Mulcan::Settings::g_vsync) ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
										   .set_desired_extent(800, 600)
										   .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
										   .build()
										   .value();

		Mulcan::Settings::g_window_extend = vkb_swapchain.extent;
		Mulcan::VKContext::g_swapchain = vkb_swapchain.swapchain;
		Mulcan::Render::g_swapchain_images = vkb_swapchain.get_images().value();
		Mulcan::Render::g_swapchain_image_views = vkb_swapchain.get_image_views().value();
		Mulcan::VKContext::g_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
		Mulcan::VKContext::g_queue_family_index = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

		VmaAllocatorCreateInfo allocator_create_info = {};
		allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
		allocator_create_info.physicalDevice = Mulcan::VKContext::g_physical_device;
		allocator_create_info.device = Mulcan::VKContext::g_device;
		allocator_create_info.instance = Mulcan::VKContext::g_instance;

		auto res = vmaCreateAllocator(&allocator_create_info, &Mulcan::g_vma_allocator);
		CHECK_VK_LOG(res);
	}

	void initializeDepthImages()
	{
		Mulcan::Render::g_depth_format = VK_FORMAT_D32_SFLOAT;

		VkExtent3D depth_extend{};
		depth_extend.depth = 1;
		depth_extend.width = Mulcan::Settings::g_window_extend.width;
		depth_extend.height = Mulcan::Settings::g_window_extend.height;

		VkImageCreateInfo depth_image_info{};
		depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depth_image_info.pNext = nullptr;
		depth_image_info.imageType = VK_IMAGE_TYPE_2D;
		depth_image_info.extent = depth_extend;
		depth_image_info.format = Mulcan::Render::g_depth_format;
		depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depth_image_info.mipLevels = 1;
		depth_image_info.arrayLayers = 1;

		VmaAllocationCreateInfo depth_alloc_info{};
		depth_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		depth_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		CHECK_VK_LOG(vmaCreateImage(Mulcan::g_vma_allocator, &depth_image_info, &depth_alloc_info, &Mulcan::Render::g_depth_image.image, &Mulcan::Render::g_depth_image.allocation, nullptr));
		VkImageViewCreateInfo depth_image_view_info{};
		depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depth_image_view_info.image = Mulcan::Render::g_depth_image.image;
		depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depth_image_view_info.format = Mulcan::Render::g_depth_format;
		depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depth_image_view_info.subresourceRange.baseMipLevel = 0;
		depth_image_view_info.subresourceRange.levelCount = 1;
		depth_image_view_info.subresourceRange.baseArrayLayer = 0;
		depth_image_view_info.subresourceRange.layerCount = 1;

		CHECK_VK_LOG(vkCreateImageView(Mulcan::VKContext::g_device, &depth_image_view_info, nullptr, &Mulcan::Render::g_depth_image_view));
	}

	void initializeCommands()
	{
		auto fence_info = MulcanInfos::createFenceInfo();
		auto semaphore_info = MulcanInfos::createSemaphoreInfo();

		for (size_t i = 0; i < Mulcan::FRAME_OVERLAP; i++)
		{
			auto command_pool_info = MulcanInfos::createCommandPoolInfo(Mulcan::VKContext::g_queue_family_index);
			CHECK_VK_LOG(vkCreateCommandPool(Mulcan::VKContext::g_device, &command_pool_info, nullptr, &Mulcan::Render::frames[i].render_pool));

			auto command_buffer_info = MulcanInfos::createCommandBufferAllocateInfo(Mulcan::Render::frames[i].render_pool, 1);
			CHECK_VK_LOG(vkAllocateCommandBuffers(Mulcan::VKContext::g_device, &command_buffer_info, &Mulcan::Render::frames[i].render_cmd));

			CHECK_VK_LOG(vkCreateFence(Mulcan::VKContext::g_device, &fence_info, nullptr, &Mulcan::Render::frames[i].render_fence));

			CHECK_VK_LOG(vkCreateSemaphore(Mulcan::VKContext::g_device, &semaphore_info, nullptr, &Mulcan::Render::frames[i].swapchain_semaphore));
			CHECK_VK_LOG(vkCreateSemaphore(Mulcan::VKContext::g_device, &semaphore_info, nullptr, &Mulcan::Render::frames[i].render_semaphore));
		}
	}

	// TODO: depth attachment
	// TODO: abstract renderpass creation.
	void initializeRenderPass()
	{
		VkAttachmentDescription color_attachment{
			.format = VK_FORMAT_B8G8R8A8_UNORM,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

		VkAttachmentDescription depth_attachment{
			.format = Mulcan::Render::g_depth_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

		VkAttachmentReference color_attachment_refence{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

		VkAttachmentReference depth_attachment_reference{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_refence,
			.pDepthStencilAttachment = &depth_attachment_reference};

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

		VkSubpassDependency depth_dependency = {};
		depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;																		// index of the subpass we're dependant on.
		depth_dependency.dstSubpass = 0;																						// index to the current subpass.
		depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; // Stages needs to be finshed within srcSubpass before dstSubpass start.
		depth_dependency.srcAccessMask = 0;
		depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; // dstStageMask tells Vulkan which stages in the destination subpass must wait until the stages listed in srcStageMask are done.
		depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachmenets;
		attachmenets[0] = color_attachment;
		attachmenets[1] = depth_attachment;

		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0] = dependency;
		dependencies[1] = depth_dependency;

		VkRenderPassCreateInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = attachmenets.size(),
			.pAttachments = &attachmenets[0],
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = dependencies.size(),
			.pDependencies = &dependencies[0],
		};

		CHECK_VK_LOG(vkCreateRenderPass(Mulcan::VKContext::g_device, &render_pass_info, nullptr, &Mulcan::Render::main_pass));
	}

	void initializeFrameBuffer()
	{
		VkFramebufferCreateInfo fb_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = Mulcan::Render::main_pass,
			.width = Mulcan::Settings::g_window_extend.width,
			.height = Mulcan::Settings::g_window_extend.height,
			.layers = 1};

		Mulcan::Render::g_main_framebuffers.resize(Mulcan::Render::g_swapchain_images.size());
		for (int i = 0; i < Mulcan::Render::g_swapchain_images.size(); i++)
		{
			std::array<VkImageView, 2> attachments{};

			attachments[0] = Mulcan::Render::g_swapchain_image_views[i];
			attachments[1] = Mulcan::Render::g_depth_image_view;

			fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			fb_info.pAttachments = attachments.data();

			CHECK_VK_LOG(vkCreateFramebuffer(Mulcan::VKContext::g_device, &fb_info, nullptr, &Mulcan::Render::g_main_framebuffers[i]));
		}
	}

	void initializeTransferBuffer()
	{
		auto fence_info = MulcanInfos::createFenceInfo();
		CHECK_VK_LOG(vkCreateFence(Mulcan::VKContext::g_device, &fence_info, nullptr, &Mulcan::buffer_transfer.fence));
		auto command_pool_info = MulcanInfos::createCommandPoolInfo(Mulcan::VKContext::g_queue_family_index);
		CHECK_VK_LOG(vkCreateCommandPool(Mulcan::VKContext::g_device, &command_pool_info, nullptr, &Mulcan::buffer_transfer.pool));

		auto command_buffer_info = MulcanInfos::createCommandBufferAllocateInfo(Mulcan::buffer_transfer.pool, 1);
		CHECK_VK_LOG(vkAllocateCommandBuffers(Mulcan::VKContext::g_device, &command_buffer_info, &Mulcan::buffer_transfer.cmd));
	}

}

void Mulcan::initialize(GLFWwindow *&window)
{
	initializeVulkan(window);
	if (Mulcan::Settings::g_has_depth)
	{
		initializeDepthImages();
	}
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

	CHECK_VK_LOG(vkBeginCommandBuffer(cmd, &cmd_info));

	VkBufferCopy copy;
	copy.dstOffset = 0;
	copy.srcOffset = 0;

	while (!Mulcan::g_transfer_buffers.empty())
	{
		copy.size = Mulcan::g_transfer_buffers.front().buffer_size;
		vkCmdCopyBuffer(cmd, Mulcan::g_transfer_buffers.front().src, Mulcan::g_transfer_buffers.front().dst, 1, &copy);
		Mulcan::g_transfer_buffers.pop();
	}

	CHECK_VK_LOG(vkEndCommandBuffer(cmd));

	vkResetFences(Mulcan::VKContext::g_device, 1, &Mulcan::buffer_transfer.fence);

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

	CHECK_VK_LOG(vkQueueSubmit(Mulcan::VKContext::g_queue, 1, &info, Mulcan::buffer_transfer.fence));

	vkWaitForFences(Mulcan::VKContext::g_device, 1, &Mulcan::buffer_transfer.fence, true, UINT32_MAX);

	vkResetCommandPool(Mulcan::VKContext::g_device, Mulcan::buffer_transfer.pool, 0);
}

// TODO: remove allocations.
void Mulcan::beginFrame()
{
	CHECK_VK_LOG(vkWaitForFences(Mulcan::VKContext::g_device, 1, &Mulcan::getCurrFrame().render_fence, true, UINT64_MAX));
	CHECK_VK_LOG(vkResetFences(Mulcan::VKContext::g_device, 1, &Mulcan::getCurrFrame().render_fence));

	CHECK_VK_LOG(vkAcquireNextImageKHR(Mulcan::VKContext::g_device, Mulcan::VKContext::g_swapchain, UINT64_MAX, Mulcan::getCurrFrame().swapchain_semaphore, nullptr, &Mulcan::Render::g_swapchain_image_index));

	CHECK_VK_LOG(vkResetCommandBuffer(Mulcan::getCurrFrame().render_cmd, 0));

	auto render_cmd_info = MulcanInfos::createCommandBufferBeginInfo(0);

	VkClearValue clear_value[2]{};

	clear_value[0].color = {{50 / 255.0f, 60 / 255.0f, 68 / 255.0f, 1.0f}};
	clear_value[1].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo main_renderpass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = Mulcan::Render::main_pass,
		.framebuffer = Mulcan::Render::g_main_framebuffers[Mulcan::Render::g_swapchain_image_index],
		.renderArea = {
			.offset = {0, 0},
			.extent = Mulcan::Settings::g_window_extend},
		.clearValueCount = 2,
		.pClearValues = &clear_value[0]};

	vkBeginCommandBuffer(Mulcan::getCurrFrame().render_cmd, &render_cmd_info);

	vkCmdBeginRenderPass(Mulcan::getCurrFrame().render_cmd, &main_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{
		.width = static_cast<float>(Mulcan::Settings::g_window_extend.width),
		.height = static_cast<float>(Mulcan::Settings::g_window_extend.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor{
		.offset = {0, 0},
		.extent = Mulcan::Settings::g_window_extend};

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
		.pSignalSemaphores = &Mulcan::getCurrFrame().render_semaphore};

	CHECK_VK_LOG(vkQueueSubmit(Mulcan::VKContext::g_queue, 1, &submit_info, Mulcan::getCurrFrame().render_fence));

	VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &Mulcan::getCurrFrame().render_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &Mulcan::VKContext::g_swapchain,
		.pImageIndices = &Mulcan::Render::g_swapchain_image_index};

	CHECK_VK_LOG(vkQueuePresentKHR(Mulcan::VKContext::g_queue, &present_info));

	Mulcan::Render::framecount++;
}

void Mulcan::setVsync(bool value)
{
	Mulcan::Settings::g_vsync = value;
}

void Mulcan::setImgui(bool value)
{
	Mulcan::Settings::g_imgui = value;
}

[[nodiscard]]
VkPipelineLayout Mulcan::buildPipelineLayout(const VkPushConstantRange &range, uint32_t range_count, uint32_t layout_count, const VkDescriptorSetLayout &layout)
{
	VkPipelineLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pushConstantRangeCount = range_count;
	info.pPushConstantRanges = &range;
	info.setLayoutCount = layout_count;
	info.pSetLayouts = &layout;

	VkPipelineLayout pipeline_layout;
	CHECK_VK_LOG(vkCreatePipelineLayout(Mulcan::VKContext::g_device, &info, nullptr, &pipeline_layout));

	return pipeline_layout;
}

// TODO: Caching support.
// TODO: Seperate shader stages.
[[nodiscard]]
VkPipeline Mulcan::buildPipeline(const Mulcan::NewPipelineDescription &new_pipeline_data)
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
	CHECK_VK_LOG(vkCreateGraphicsPipelines(Mulcan::VKContext::g_device, nullptr, 1, &info, nullptr, &pipeline));

	vkDestroyShaderModule(Mulcan::VKContext::g_device, new_pipeline_data.vertex_shader, nullptr);
	vkDestroyShaderModule(Mulcan::VKContext::g_device, new_pipeline_data.fragment_shader, nullptr);

	return pipeline;
}

bool Mulcan::addTransferBuffer(const Mulcan::TransferBuffer &transfer_buffer)
{
	if (transfer_buffer.buffer_size == 0 || transfer_buffer.src == VK_NULL_HANDLE || transfer_buffer.dst == VK_NULL_HANDLE)
	{
		return false;
	}
	Mulcan::g_transfer_buffers.push(transfer_buffer);

	return true;
}

VkCommandBuffer Mulcan::getCurrCommand()
{
	return Mulcan::getCurrFrame().render_cmd;
}

VkRenderPass Mulcan::getMainPass()
{
	return Mulcan::Render::main_pass;
}

bool Mulcan::loadShaderModule(const char *filePath, VkShaderModule *out_shader_module)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	size_t fileSize = (size_t)file.tellg();

	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);

	file.read((char *)buffer.data(), fileSize);

	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Mulcan::VKContext::g_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return false;
	}
	*out_shader_module = shaderModule;
	return true;
}

// TODO: deletion queue
void Mulcan::shutdown()
{
	vkDeviceWaitIdle(Mulcan::VKContext::g_device);

	for (auto &framebuffer : Mulcan::Render::g_main_framebuffers)
	{
		vkDestroyFramebuffer(Mulcan::VKContext::g_device, framebuffer, nullptr);
	}

	vkDestroyRenderPass(Mulcan::VKContext::g_device, Mulcan::Render::main_pass, nullptr);
	for (auto &frame : Render::frames)
	{
		vkDestroyCommandPool(Mulcan::VKContext::g_device, frame.render_pool, nullptr);
		vkDestroyFence(Mulcan::VKContext::g_device, frame.render_fence, nullptr);
		vkDestroySemaphore(Mulcan::VKContext::g_device, frame.render_semaphore, nullptr);
		vkDestroySemaphore(Mulcan::VKContext::g_device, frame.swapchain_semaphore, nullptr);
	}
	for (auto &view : Mulcan::Render::g_swapchain_image_views)
	{
		vkDestroyImageView(Mulcan::VKContext::g_device, view, nullptr);
	}
	vmaDestroyAllocator(Mulcan::g_vma_allocator);
	vkDestroySwapchainKHR(Mulcan::VKContext::g_device, Mulcan::VKContext::g_swapchain, nullptr);
	vkDestroyDevice(Mulcan::VKContext::g_device, nullptr);
	vkDestroySurfaceKHR(Mulcan::VKContext::g_instance, Mulcan::VKContext::g_surface, nullptr);
	vkb::destroy_debug_utils_messenger(Mulcan::VKContext::g_instance, Mulcan::VKContext::g_debug_messenger, nullptr);
	vkDestroyInstance(Mulcan::VKContext::g_instance, nullptr);
}
