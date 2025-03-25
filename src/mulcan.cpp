#include "mulcan.hpp"

Mulcan::MulcanResult Mulcan::initialize(VkSurfaceKHR surface)
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

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice physical_device = selector
                                              .set_minimum_version(1, 3)
                                              .set_surface(Mulcan::g_surface)
                                              .select()
                                              .value();

    vkb::DeviceBuilder deviceBuilder{physical_device};

    vkb::Device vkb_device = deviceBuilder.build().value();

    Mulcan::g_device = vkb_device.device;
    Mulcan::g_physical_device = physical_device.physical_device;

    vkb::SwapchainBuilder swapchainBuilder{Mulcan::g_physical_device, Mulcan::g_device, Mulcan::g_surface};

    Mulcan::g_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain = swapchainBuilder
                                       .set_desired_format(VkSurfaceFormatKHR{.format = Mulcan::g_swapchain_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                       .set_desired_present_mode((vsync) ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
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

void Mulcan::cleanup()
{
    vmaDestroyAllocator(Mulcan::g_vma_allocator);
    vkDestroySwapchainKHR(Mulcan::g_device, Mulcan::g_swapchain, nullptr);
    vkDestroyDevice(Mulcan::g_device, nullptr);
    vkDestroySurfaceKHR(Mulcan::g_instance, Mulcan::g_surface, nullptr);
    vkb::destroy_debug_utils_messenger(Mulcan::g_instance, Mulcan::g_debug_messenger, nullptr);
    vkDestroyInstance(Mulcan::g_instance, nullptr);
}
