#pragma once

#include <iostream>
#include <spdlog/spdlog.h>

#ifdef NDEBUG
do
{
} while (0)
#else
#define CHECK_VK_LOG(res)                                                      \
    if (res != VK_SUCCESS)                                                     \
    {                                                                          \
        spdlog::error("Vulkan error: {} | File: {} | Line: {} | Function: {}", \
                      #res, __FILE__, __LINE__, __func__);                     \
        std::cout << "Hello";                                                  \
        abort();                                                               \
    }
#endif // DEBUG

#ifdef NDEBUG
#define LOG(msg) \
    do           \
    {            \
    } while (0)
#else
#define LOG(msg) spdlog::info(msg)
#endif
