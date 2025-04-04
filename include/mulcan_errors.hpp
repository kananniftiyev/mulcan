#pragma once

#include <spdlog/spdlog.h>

namespace Mulcan
{
	enum class MulcanResult : size_t
	{
		M_BUFFER_ERROR = 1,
		M_INIT_ERROR = 2,
		M_UNKNOWN_ERROR = 3,
		M_SUCCESS = 4,
		M_VMA_ERROR = 5,
		M_COMMAND_INIT_ERROR = 6,
		M_RENDERPASS_ERROR = 7,
		M_FRAMEBUFFER_INIT_ERROR = 8,
		M_SYNC_ERROR = 9,
		M_QUEUE_ERROR = 10,
		M_EMPTY_VARS_OR_BUFFERS = 11,

	};

	void errorResultToMessage();
} // namespace Mulcan

#define CHECK_VK(res, error_enum) \
	if (res != VK_SUCCESS)        \
	{                             \
		return error_enum;        \
	}

#define CHECK_VK_LOG(res, message)                            \
	if (res != VK_SUCCESS)                                    \
	{                                                         \
		spdlog::error("Vulkan error: {}: {}", #res, message); \
		abort();                                              \
	}