#define VMA_IMPLEMENTATION
#define VMA_DEBUG_DETECT_CORRUPTION 1
#define VMA_DEBUG_MARGIN 16
#include "mulcan.hpp"

namespace Mulcan
{

	constexpr size_t FRAME_OVERLAP = 2;

	namespace Settings
	{
		bool gVsync = true;
		bool gImgui = false;
		bool gHasDepth = true;
		VkExtent2D gWindowExtend{.width = 800, .height = 600};
	}

	namespace VKContext
	{
		VkDevice gDevice;
		VkInstance gInstance;
		VkPhysicalDevice gPhysicalDevice;
		VkQueue gQueue;
		uint32_t gQueueFamilyIndex;
		VkSurfaceKHR gSurface;
		VkSwapchainKHR gSwapchain;
		VkDebugUtilsMessengerEXT gDebugMessenger;
	}

	VmaAllocator gVmaAllocator;

	namespace Flags
	{
		bool hasRunTransferCommands = false;
	}

	namespace Render
	{
		std::vector<VkImage> gSwapchainImages;
		std::vector<VkImageView> gSwapchainImageViews;
		VkFormat gSwapchainFormat;
		VkFormat gDepthFormat;
		std::array<FrameData, FRAME_OVERLAP> gFrames;
		std::vector<VkFramebuffer> gMainFramebuffers;
		AllocatedImage gDepthImage;
		VkImageView gDepthImageView;
		uint32_t gFramecount = 0;
		uint32_t gSwapchainImageIndex;
	}

	namespace Pipelines
	{
		VkPipeline ObjectMainPipeline;
		VkPipeline ObjectShadowPipeline;
		VkPipeline ObjectSkyboxPipeline;
	} // namespace Pipelines

	namespace PipelineLayouts
	{
		VkPipelineLayout ObjectMainLayout;
	} // namespace PipelineLayouts

	namespace RenderPasses
	{
		VkRenderPass gObjectMainRenderPass;
	} // namespace RenderPasses

	ImmediateSubmitData gBufferTransfer;
	std::vector<TransferBuffer> gTransferBuffers;
	std::vector<VkBuffer> gBufferDeletionList;

	FrameData &getCurrFrame() { return Render::gFrames[Render::gFramecount % FRAME_OVERLAP]; }

	static uint32_t gUniqueHandle = 0;

	AllocatedBuffer gUnifromCameraBuffer;

	std::unique_ptr<Camera> gCamera;

	entt::registry gRegistery;

}

namespace
{
	void initializeSettings(uint32_t pWidth, uint32_t pHeigth)
	{
		// Settings
		Mulcan::Settings::gWindowExtend.width = pWidth;
		Mulcan::Settings::gWindowExtend.height = pHeigth;

		// containers
		Mulcan::gTransferBuffers.reserve(100);
	}

	void initializeSwapchain()
	{
		vkb::SwapchainBuilder swapchainBuilder{Mulcan::VKContext::gPhysicalDevice, Mulcan::VKContext::gDevice, Mulcan::VKContext::gSurface};

		Mulcan::Render::gSwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkb_swapchain = swapchainBuilder
										   .set_desired_format(VkSurfaceFormatKHR{.format = Mulcan::Render::gSwapchainFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
										   .set_desired_present_mode((Mulcan::Settings::gVsync) ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
										   .set_desired_extent(Mulcan::Settings::gWindowExtend.width, Mulcan::Settings::gWindowExtend.height)
										   .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
										   .build()
										   .value();

		Mulcan::Settings::gWindowExtend = vkb_swapchain.extent;
		Mulcan::VKContext::gSwapchain = vkb_swapchain.swapchain;
		Mulcan::Render::gSwapchainImages = vkb_swapchain.get_images().value();
		Mulcan::Render::gSwapchainImageViews = vkb_swapchain.get_image_views().value();
	}

	void initializeVulkan(SDL_Window *&pWindow)
	{
		vkb::InstanceBuilder instance_builder;
		auto inst_ret = instance_builder.request_validation_layers()
							.require_api_version(1, 2, 0)
							.use_default_debug_messenger()
							.set_app_name("Mulcan")
							.build();

		auto vkb_inst = inst_ret.value();

		Mulcan::VKContext::gInstance = vkb_inst.instance;
		Mulcan::VKContext::gDebugMessenger = vkb_inst.debug_messenger;

		auto sdl_res = SDL_Vulkan_CreateSurface(pWindow, Mulcan::VKContext::gInstance, nullptr, &Mulcan::VKContext::gSurface);

		if (!sdl_res)
		{
			spdlog::error("Could not create window");
			abort();
		}

		vkb::PhysicalDeviceSelector selector{vkb_inst};
		vkb::PhysicalDevice physical_device = selector
												  .set_minimum_version(1, 2)
												  .set_surface(Mulcan::VKContext::gSurface)
												  .select()
												  .value();

		vkb::DeviceBuilder deviceBuilder{physical_device};

		vkb::Device vkb_device = deviceBuilder.build().value();

		Mulcan::VKContext::gDevice = vkb_device.device;
		Mulcan::VKContext::gPhysicalDevice = physical_device.physical_device;
		Mulcan::VKContext::gQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
		Mulcan::VKContext::gQueueFamilyIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

		VmaAllocatorCreateInfo allocator_create_info = {};
		allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
		allocator_create_info.physicalDevice = Mulcan::VKContext::gPhysicalDevice;
		allocator_create_info.device = Mulcan::VKContext::gDevice;
		allocator_create_info.instance = Mulcan::VKContext::gInstance;

		auto res = vmaCreateAllocator(&allocator_create_info, &Mulcan::gVmaAllocator);
		CHECK_VK_LOG(res);
	}

	void initializeDepthImages()
	{
		Mulcan::Render::gDepthFormat = VK_FORMAT_D32_SFLOAT;

		VkExtent3D depth_extend{};
		depth_extend.depth = 1;
		depth_extend.width = Mulcan::Settings::gWindowExtend.width;
		depth_extend.height = Mulcan::Settings::gWindowExtend.height;

		VkImageCreateInfo depth_image_info{};
		depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depth_image_info.pNext = nullptr;
		depth_image_info.imageType = VK_IMAGE_TYPE_2D;
		depth_image_info.extent = depth_extend;
		depth_image_info.format = Mulcan::Render::gDepthFormat;
		depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depth_image_info.mipLevels = 1;
		depth_image_info.arrayLayers = 1;

		VmaAllocationCreateInfo depth_alloc_info{};
		depth_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		depth_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		CHECK_VK_LOG(vmaCreateImage(Mulcan::gVmaAllocator, &depth_image_info, &depth_alloc_info, &Mulcan::Render::gDepthImage.image, &Mulcan::Render::gDepthImage.allocation, nullptr));
		VkImageViewCreateInfo depth_image_view_info{};
		depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depth_image_view_info.image = Mulcan::Render::gDepthImage.image;
		depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depth_image_view_info.format = Mulcan::Render::gDepthFormat;
		depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depth_image_view_info.subresourceRange.baseMipLevel = 0;
		depth_image_view_info.subresourceRange.levelCount = 1;
		depth_image_view_info.subresourceRange.baseArrayLayer = 0;
		depth_image_view_info.subresourceRange.layerCount = 1;

		CHECK_VK_LOG(vkCreateImageView(Mulcan::VKContext::gDevice, &depth_image_view_info, nullptr, &Mulcan::Render::gDepthImageView));
	}

	void initializeCommands()
	{
		auto fence_info = MulcanInfos::createFenceInfo();
		auto semaphore_info = MulcanInfos::createSemaphoreInfo();

		for (size_t i = 0; i < Mulcan::FRAME_OVERLAP; i++)
		{
			auto command_pool_info = MulcanInfos::createCommandPoolInfo(Mulcan::VKContext::gQueueFamilyIndex);
			CHECK_VK_LOG(vkCreateCommandPool(Mulcan::VKContext::gDevice, &command_pool_info, nullptr, &Mulcan::Render::gFrames[i].render_pool));

			auto command_buffer_info = MulcanInfos::createCommandBufferAllocateInfo(Mulcan::Render::gFrames[i].render_pool, 1);
			CHECK_VK_LOG(vkAllocateCommandBuffers(Mulcan::VKContext::gDevice, &command_buffer_info, &Mulcan::Render::gFrames[i].render_cmd));

			CHECK_VK_LOG(vkCreateFence(Mulcan::VKContext::gDevice, &fence_info, nullptr, &Mulcan::Render::gFrames[i].render_fence));

			CHECK_VK_LOG(vkCreateSemaphore(Mulcan::VKContext::gDevice, &semaphore_info, nullptr, &Mulcan::Render::gFrames[i].swapchain_semaphore));
			CHECK_VK_LOG(vkCreateSemaphore(Mulcan::VKContext::gDevice, &semaphore_info, nullptr, &Mulcan::Render::gFrames[i].render_semaphore));
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
			.format = Mulcan::Render::gDepthFormat,
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

		CHECK_VK_LOG(vkCreateRenderPass(Mulcan::VKContext::gDevice, &render_pass_info, nullptr, &Mulcan::RenderPasses::gObjectMainRenderPass));
	}

	void initializeFrameBuffer()
	{
		VkFramebufferCreateInfo fb_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = Mulcan::RenderPasses::gObjectMainRenderPass,
			.width = Mulcan::Settings::gWindowExtend.width,
			.height = Mulcan::Settings::gWindowExtend.height,
			.layers = 1};

		Mulcan::Render::gMainFramebuffers.resize(Mulcan::Render::gSwapchainImages.size());
		for (size_t i = 0; i < Mulcan::Render::gSwapchainImages.size(); i++)
		{
			std::array<VkImageView, 2> attachments{};

			attachments[0] = Mulcan::Render::gSwapchainImageViews[i];
			attachments[1] = Mulcan::Render::gDepthImageView;

			fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			fb_info.pAttachments = attachments.data();

			CHECK_VK_LOG(vkCreateFramebuffer(Mulcan::VKContext::gDevice, &fb_info, nullptr, &Mulcan::Render::gMainFramebuffers[i]));
		}
	}

	void initializeTransferBuffer()
	{
		auto fence_info = MulcanInfos::createFenceInfo();
		CHECK_VK_LOG(vkCreateFence(Mulcan::VKContext::gDevice, &fence_info, nullptr, &Mulcan::gBufferTransfer.fence));
		auto command_pool_info = MulcanInfos::createCommandPoolInfo(Mulcan::VKContext::gQueueFamilyIndex);
		CHECK_VK_LOG(vkCreateCommandPool(Mulcan::VKContext::gDevice, &command_pool_info, nullptr, &Mulcan::gBufferTransfer.pool));

		auto command_buffer_info = MulcanInfos::createCommandBufferAllocateInfo(Mulcan::gBufferTransfer.pool, 1);
		CHECK_VK_LOG(vkAllocateCommandBuffers(Mulcan::VKContext::gDevice, &command_buffer_info, &Mulcan::gBufferTransfer.cmd));
	}

}

namespace
{
	void transitionImageToPresentLayout(VkCommandBuffer pCmd, VkImage pImage, const VkQueue &pQueue)
	{
		VkImageMemoryBarrier info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		info.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		info.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		info.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		info.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		info.image = pImage;
		info.subresourceRange = {};
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(pCmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &info);
	}

	void runTransferBufferCommand()
	{
		auto cmd = Mulcan::gBufferTransfer.cmd;

		auto cmd_info = MulcanInfos::createCommandBufferBeginInfo(0);

		CHECK_VK_LOG(vkBeginCommandBuffer(cmd, &cmd_info));

		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;

		for (const auto &buf : Mulcan::gTransferBuffers)
		{
			copy.size = buf.buffer_size;
			vkCmdCopyBuffer(cmd, buf.src, buf.dst, 1, &copy);
		}

		CHECK_VK_LOG(vkEndCommandBuffer(cmd));

		vkResetFences(Mulcan::VKContext::gDevice, 1, &Mulcan::gBufferTransfer.fence);

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

		CHECK_VK_LOG(vkQueueSubmit(Mulcan::VKContext::gQueue, 1, &info, Mulcan::gBufferTransfer.fence));

		vkWaitForFences(Mulcan::VKContext::gDevice, 1, &Mulcan::gBufferTransfer.fence, true, UINT32_MAX);

		vkResetCommandPool(Mulcan::VKContext::gDevice, Mulcan::gBufferTransfer.pool, 0);

		for (auto &buf : Mulcan::gTransferBuffers)
		{
			spdlog::info("Deleted Src buffer, size of buffer: {}", sizeof(buf.buffer_size));
			vmaDestroyBuffer(Mulcan::gVmaAllocator, buf.src, nullptr);
		}
	}
}

namespace
{

	void InitObjectMainPipeline()
	{
		VkDescriptorSetLayoutBinding cameraBinding{};
		cameraBinding.binding = 0;
		cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraBinding.descriptorCount = 1;
		cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		Mulcan::Descriptor::DoubleBufferedDescriptorBuildCtx cameraBuildCtx;
		Mulcan::Descriptor::addBindingToSetDB(cameraBinding, &cameraBuildCtx);
		Mulcan::Descriptor::addBufferToSetDB(sizeof(GPUCameraData), &cameraBuildCtx);
		auto cameraCtx = Mulcan::Descriptor::buildSetDB(Mulcan::VKContext::gDevice, Mulcan::gVmaAllocator, &cameraBuildCtx);

		Mulcan::Pipeline pipeline{Mulcan::VKContext::gDevice, Mulcan::RenderPasses::gObjectMainRenderPass};
		Mulcan::gCamera = std::make_unique<Camera>(cameraCtx, Mulcan::gVmaAllocator);

		VkPushConstantRange pushConsant{};
		pushConsant.size = sizeof(Mulcan::MeshPushConstants);
		pushConsant.offset = 0;
		pushConsant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		pipeline.CreatePipelineLayout({pushConsant}, {cameraCtx.layout});
		pipeline.CreatePipeline("./shaders/cube-v.spv", "./shaders/cube-f.spv");

		Mulcan::Pipelines::ObjectMainPipeline = pipeline.GetPipeline();
		Mulcan::PipelineLayouts::ObjectMainLayout = pipeline.GetPipelineLayout();
	}

	void InitObjectShadowPipeline()
	{
	}

	void InitObjectSkyboxPipeline()
	{
	}
}

void Mulcan::initialize(SDL_Window *&pWindow, uint32_t pWidth, uint32_t pHeigth)
{
	initializeSettings(pWidth, pHeigth);
	initializeVulkan(pWindow);
	if (Mulcan::Settings::gHasDepth)
	{
		initializeDepthImages();
	}
	initializeCommands();
	initializeRenderPass();
	initializeFrameBuffer();
	initializeTransferBuffer();

	// Pipelines
	InitObjectMainPipeline();
	InitObjectShadowPipeline();
	InitObjectSkyboxPipeline();
}

// TODO: remove allocations.
void Mulcan::beginFrame()
{
	if (!Mulcan::Flags::hasRunTransferCommands)
	{
		runTransferBufferCommand();
		Mulcan::Flags::hasRunTransferCommands = true;
	}

	CHECK_VK_LOG(vkWaitForFences(Mulcan::VKContext::gDevice, 1, &Mulcan::getCurrFrame().render_fence, true, UINT64_MAX));
	CHECK_VK_LOG(vkResetFences(Mulcan::VKContext::gDevice, 1, &Mulcan::getCurrFrame().render_fence));

	CHECK_VK_LOG(vkAcquireNextImageKHR(Mulcan::VKContext::gDevice, Mulcan::VKContext::gSwapchain, UINT64_MAX, Mulcan::getCurrFrame().swapchain_semaphore, nullptr, &Mulcan::Render::gSwapchainImageIndex));

	CHECK_VK_LOG(vkResetCommandBuffer(Mulcan::getCurrFrame().render_cmd, 0));

	auto render_cmd_info = MulcanInfos::createCommandBufferBeginInfo(0);

	VkClearValue clear_value[2]{};

	clear_value[0].color = {{50 / 255.0f, 60 / 255.0f, 68 / 255.0f, 1.0f}};
	clear_value[1].depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo main_renderpass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = Mulcan::RenderPasses::gObjectMainRenderPass,
		.framebuffer = Mulcan::Render::gMainFramebuffers[Mulcan::Render::gSwapchainImageIndex],
		.renderArea = {
			.offset = {0, 0},
			.extent = Mulcan::Settings::gWindowExtend},
		.clearValueCount = 2,
		.pClearValues = &clear_value[0]};

	vkBeginCommandBuffer(Mulcan::getCurrFrame().render_cmd, &render_cmd_info);

	vkCmdBeginRenderPass(Mulcan::getCurrFrame().render_cmd, &main_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{
		.width = static_cast<float>(Mulcan::Settings::gWindowExtend.width),
		.height = static_cast<float>(Mulcan::Settings::gWindowExtend.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor{
		.offset = {0, 0},
		.extent = Mulcan::Settings::gWindowExtend};

	vkCmdSetViewport(Mulcan::getCurrFrame().render_cmd, 0, 1, &viewport);
	vkCmdSetScissor(Mulcan::getCurrFrame().render_cmd, 0, 1, &scissor);
}

// TODO: Refactor to infos.
void Mulcan::endFrame()
{
	vkCmdEndRenderPass(Mulcan::getCurrFrame().render_cmd);

	transitionImageToPresentLayout(Mulcan::getCurrFrame().render_cmd, Mulcan::Render::gSwapchainImages[Mulcan::Render::gSwapchainImageIndex], Mulcan::VKContext::gQueue);

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

	CHECK_VK_LOG(vkQueueSubmit(Mulcan::VKContext::gQueue, 1, &submit_info, Mulcan::getCurrFrame().render_fence));

	VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &Mulcan::getCurrFrame().render_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &Mulcan::VKContext::gSwapchain,
		.pImageIndices = &Mulcan::Render::gSwapchainImageIndex};

	CHECK_VK_LOG(vkQueuePresentKHR(Mulcan::VKContext::gQueue, &present_info));

	Mulcan::Render::gFramecount++;
}

inline uint32_t GenerateUniqueHandle()
{
	return Mulcan::gUniqueHandle++;
}

void Mulcan::RenderWorldSystem()
{
	VkDeviceSize offsets[1]{0};
	vkCmdBindPipeline(Mulcan::getCurrCommand(), VK_PIPELINE_BIND_POINT_GRAPHICS, Mulcan::Pipelines::ObjectMainPipeline);

	auto worldView = Mulcan::gRegistery.view<const Mulcan::MeshComponent, Mulcan::TransformComponent>();

	for (auto [entity, mesh, transform] : worldView.each())
	{
		if (!mesh.isVisible)
		{
			continue;
		}

		vkCmdBindVertexBuffers(Mulcan::getCurrCommand(), 0, 1, &mesh.vertexBuffer, offsets);
		vkCmdBindIndexBuffer(Mulcan::getCurrCommand(), mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		transform.rotation = glm::vec3(0.0f, glm::radians(Mulcan::Render::gFramecount * 0.4f), 0.0f);

		auto model = CreateModelMatrix(transform.position, transform.rotation, transform.scale);

		Mulcan::MeshPushConstants constants;
		constants.render_matrix = model;

		gCamera->changeAspect((float)Mulcan::Settings::gWindowExtend.width, (float)Mulcan::Settings::gWindowExtend.width);

		gCamera->UpdateCamera(Mulcan::Render::gFramecount % FRAME_OVERLAP);

		vkCmdBindDescriptorSets(Mulcan::getCurrCommand(), VK_PIPELINE_BIND_POINT_GRAPHICS, Mulcan::PipelineLayouts::ObjectMainLayout, 0, 1, &gCamera->mCtx.set[Mulcan::Render::gFramecount % Mulcan::FRAME_OVERLAP], 0, nullptr);
		vkCmdPushConstants(Mulcan::getCurrCommand(), Mulcan::PipelineLayouts::ObjectMainLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mulcan::MeshPushConstants), &constants);

		vkCmdDrawIndexed(Mulcan::getCurrCommand(), mesh.indexCount, 1, 0, 0, 0);
	}
}

bool Mulcan::SpawnSampleCube()
{
	const std::vector<Mulcan::Vertex> vertices = {
		// Position                 // Color                // TexCoord           // Normal
		{{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},	// 0: v1 (red)
		{{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},	// 1: v2 (green)
		{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},		// 2: v3 (blue)
		{{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},	// 3: v4 (yellow)
		{{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},	// 4: v5 (magenta)
		{{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, // 5: v6 (cyan)
		{{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},	// 6: v7 (white)
		{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}	// 7: v8 (gray)
	};

	const std::vector<uint32_t> indices = {
		// Top face (Y+)
		0, 4, 6,
		0, 6, 2,

		// Front face (Z+)
		2, 6, 7,
		2, 7, 3,

		// Left face (X-)
		6, 4, 5,
		6, 5, 7,

		// Bottom face (Y-)
		3, 7, 5,
		3, 5, 1,

		// Right face (X+)
		0, 2, 3,
		0, 3, 1,

		// Back face (Z-)
		0, 1, 5,
		0, 5, 4};

	auto handle = GenerateUniqueHandle();

	VkBuffer vertexBuffer, indexBuffer;
	createTransferBuffer(vertices.data(), vertices.size() * sizeof(Mulcan::Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertexBuffer);
	Mulcan::createTransferBuffer(indices.data(), indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indexBuffer);

	const auto entity = Mulcan::gRegistery.create();

	MeshComponent mesh{
		.name = "cube", // TODO: Increment name if same name exists
		.vertexBuffer = vertexBuffer,
		.indexBuffer = indexBuffer,
		.indexCount = 36,
		.isVisible = true};

	TransformComponent transform{
		.position = glm::vec3(0.0f),
		.rotation = glm::vec3(0.0f),
		.scale = glm::vec3(1.0f),
		.isChanged = true};

	Mulcan::gRegistery.emplace<Mulcan::MeshComponent>(entity, mesh);
	Mulcan::gRegistery.emplace<Mulcan::TransformComponent>(entity, transform);

	Mulcan::addDestroyBuffer(vertexBuffer);
	Mulcan::addDestroyBuffer(indexBuffer);

	return true;
}

bool Mulcan::SpawnCustomModel(const char *pFilePath)
{
	return false;
}

void Mulcan::RemoveModel(std::string name)
{
	auto deleteView = Mulcan::gRegistery.view<MeshComponent>();
	for (auto [entity, mesh] : deleteView.each())
	{
		if (name == mesh.name)
		{
			Mulcan::gRegistery.destroy(entity);
		}
	}
}

inline glm::mat4 Mulcan::CreateModelMatrix(const glm::vec3 &pPos, const glm::vec3 &pRotation, const glm::vec3 &pScale)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, pScale); // 1. Scale
	model = glm::rotate(model, pRotation.z, {0, 0, 1});
	model = glm::rotate(model, pRotation.y, {0, 1, 0});
	model = glm::rotate(model, pRotation.x, {1, 0, 0});
	model = glm::translate(model, pPos); // 5. Translate

	return model;
}

void Mulcan::setVsync(bool pValue)
{
	Mulcan::Settings::gVsync = pValue;
}

void Mulcan::setImgui(bool pValue)
{
	Mulcan::Settings::gImgui = pValue;
}

bool Mulcan::addTransferBuffer(const Mulcan::TransferBuffer &pTransferBuffer)
{
	if (pTransferBuffer.buffer_size == 0 || pTransferBuffer.src == VK_NULL_HANDLE || pTransferBuffer.dst == VK_NULL_HANDLE)
	{
		return false;
	}
	Mulcan::gTransferBuffers.push_back(pTransferBuffer);

	return true;
}

void Mulcan::createTransferBuffer(const void *pData, size_t size, VkBufferUsageFlags pFlag, VkBuffer *outBuffer)
{
	// CPU side
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo vma_alloc_info{};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	vma_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	AllocatedBuffer staging_buffer;
	CHECK_VK_LOG(vmaCreateBuffer(Mulcan::gVmaAllocator, &buffer_info, &vma_alloc_info, &staging_buffer.buffer, &staging_buffer.allocation, nullptr));

	void *data;
	vmaMapMemory(Mulcan::gVmaAllocator, staging_buffer.allocation, &data);

	memcpy(data, pData, size);

	vmaUnmapMemory(Mulcan::gVmaAllocator, staging_buffer.allocation);

	// GPU side
	VkBufferCreateInfo gpu_buffer_info{};
	gpu_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	gpu_buffer_info.size = size;
	gpu_buffer_info.usage = pFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	vma_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vma_alloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	AllocatedBuffer gpu_buffer;

	CHECK_VK_LOG(vmaCreateBuffer(Mulcan::gVmaAllocator, &gpu_buffer_info, &vma_alloc_info, &gpu_buffer.buffer, &gpu_buffer.allocation, nullptr));

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

	*outBuffer = gpu_buffer.buffer;
}

void Mulcan::recreateSwapchain(uint32_t pWidth, uint32_t pHeight)
{
	Mulcan::Settings::gWindowExtend.width = pWidth;
	Mulcan::Settings::gWindowExtend.height = pHeight;
	vkDestroySwapchainKHR(Mulcan::VKContext::gDevice, Mulcan::VKContext::gSwapchain, nullptr);
	initializeSwapchain();
	initializeDepthImages();
	initializeFrameBuffer();

	LOG("Recreated the Swapchain");
}

void Mulcan::addDestroyBuffer(VkBuffer &pBuffer)
{
	if (pBuffer == VK_NULL_HANDLE)
	{
		spdlog::error("Buffer is null. Cannot delete null buffers!!!");
		return;
	}

	Mulcan::gBufferDeletionList.push_back(pBuffer);
}

// TODO: deletion queue
void Mulcan::shutdown()
{
	vkDeviceWaitIdle(Mulcan::VKContext::gDevice);

	for (auto &buffer : Mulcan::gBufferDeletionList)
	{
		LOG("destroying buffer");
		vmaDestroyBuffer(Mulcan::gVmaAllocator, buffer, nullptr);
	}

	vmaDestroyImage(Mulcan::gVmaAllocator, Mulcan::Render::gDepthImage.image, nullptr);
	LOG("deleted image");

	for (auto &framebuffer : Mulcan::Render::gMainFramebuffers)
	{
		vkDestroyFramebuffer(Mulcan::VKContext::gDevice, framebuffer, nullptr);
	}

	vkDestroyRenderPass(Mulcan::VKContext::gDevice, Mulcan::RenderPasses::gObjectMainRenderPass, nullptr);
	for (auto &frame : Render::gFrames)
	{
		vkDestroyCommandPool(Mulcan::VKContext::gDevice, frame.render_pool, nullptr);
		vkDestroyFence(Mulcan::VKContext::gDevice, frame.render_fence, nullptr);
		vkDestroySemaphore(Mulcan::VKContext::gDevice, frame.render_semaphore, nullptr);
		vkDestroySemaphore(Mulcan::VKContext::gDevice, frame.swapchain_semaphore, nullptr);
	}
	for (auto &view : Mulcan::Render::gSwapchainImageViews)
	{
		vkDestroyImageView(Mulcan::VKContext::gDevice, view, nullptr);
	}
	vmaDestroyAllocator(Mulcan::gVmaAllocator);
	vkDestroySwapchainKHR(Mulcan::VKContext::gDevice, Mulcan::VKContext::gSwapchain, nullptr);
	vkDestroyDevice(Mulcan::VKContext::gDevice, nullptr);
	vkDestroySurfaceKHR(Mulcan::VKContext::gInstance, Mulcan::VKContext::gSurface, nullptr);
	vkb::destroy_debug_utils_messenger(Mulcan::VKContext::gInstance, Mulcan::VKContext::gDebugMessenger, nullptr);
	vkDestroyInstance(Mulcan::VKContext::gInstance, nullptr);
}

// Getters
VkCommandBuffer Mulcan::getCurrCommand()
{
	return Mulcan::getCurrFrame().render_cmd;
}

VkRenderPass Mulcan::getMainPass()
{
	return Mulcan::RenderPasses::gObjectMainRenderPass;
}

VkDevice &Mulcan::getDevice()
{
	return Mulcan::VKContext::gDevice;
}

VmaAllocator &Mulcan::getAllocator()
{
	return Mulcan::gVmaAllocator;
}
