#pragma once

namespace Mulcan::Utils {
	bool loadShaderModule(const char* filePath, VkShaderModule* out_shader_module);
}