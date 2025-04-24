#pragma once

#include <array>

template <typename T, int N>
struct FrameResource
{
    std::array<T, N> data;
};
