#pragma once

#include <iostream>
#include <spdlog/spdlog.h>

#define CHECK_VK_LOG(res)                                                              \
    if (res != VK_SUCCESS)                                                             \
    {                                                                                  \
        spdlog::error("Vulkan error: {} | File: {} | Line: {} | Function: {}",         \
                      #res, __FILE__, __LINE__, __func__);                             \
        std::cout << "Hello";                                                          \
        abort();                                                                       \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
        spdlog::info("Vulkan call succeeded: {} | File: {} | Line: {} | Function: {}", \
                     #res, __FILE__, __LINE__, __func__);                              \
    }
