#pragma once

namespace Mulcan
{
    enum class MulcanResult : size_t
    {
        M_BUFFER_ERROR = 1,
        M_INIT_ERROR = 2,
        M_UNKNOWN_ERROR = 3,
        M_SUCCESS = 4,
        M_VMA_ERROR = 5
    };

    void errorResultToMessage();
} // namespace Mulcan

#define CHECK_VK(res, error_enum) \
    if (res != VK_SUCCESS)        \
    {                             \
        return error_enum;        \
    }