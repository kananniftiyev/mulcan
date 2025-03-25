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
    static bool vsync = true;
    constexpr size_t FRAME_OVERLAP = 2;

    struct FrameData
    {
        VkFence render_fence;
        VkSemaphore swapchain_semaphore, render_semaphore;
        VkCommandBuffer render_cmd;
        VkCommandPool render_pool;
    };

    std::array<FrameData, FRAME_OVERLAP> frames;

    MulcanResult initialize(VkSurfaceKHR surface);
    MulcanResult initializeCommands();
    MulcanResult initializeRenderPass();
    MulcanResult initializeFrameBuffer();

    void beginFrame();
    void endFrame();

    // Settigns functions
    void setVsync(bool value);
    void setFrameInFlight(size_t value);
    void setImgui(bool value);

    void cleanup();
}