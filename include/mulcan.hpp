#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <vector>
#include "mulcan_errors.hpp"

namespace Mulcan
{
    static VkDevice g_device;
    static VkInstance g_instance;
    static VkPhysicalDevice g_physical_device;
    static VkQueue g_queue;
    static uint32_t g_queue_family_index;
    static VkSurfaceKHR g_surface;
    static VkSwapchainKHR g_swapchain;
    static VkDebugUtilsMessengerEXT g_debug_messenger;
    static std::vector<VkImage> g_swapchain_images;
    static std::vector<VkImageView> g_swapchain_image_views;
    static VmaAllocator g_vma_allocator;
    static VkFormat g_swapchain_format;
    static VkExtent2D g_window_extend;

    MulcanResult initialize(VkSurfaceKHR surface);
    void initializeCommands();
    void initializeRenderPass();
    void initializeFrameBuffer();

    void beginFrame();
    void endFrame();

    void cleanup();
}