#pragma once

namespace Mulcan
{
    enum class MulcanResult
    {
        M_BUFFER_ERROR,
        M_INIT_ERROR,
        M_UNKNOWN_ERROR,
        M_SUCCESS,

    };

    void errorResultToMessage();
} // namespace Mulcan
