#pragma once

#ifdef _WIN32
#include <vma/vk_mem_alloc.h>
#endif // _WIN32
#ifdef __linux__
#include <vk_mem_alloc.h>
#endif // _UNIX
#include <vulkan/vulkan.h>

struct AllocatedImage
{
    VkImage image;
    VmaAllocation allocation;
};

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};