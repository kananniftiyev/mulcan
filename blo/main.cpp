#define GLFW_INCLUDE_VULKAN
#include "mulcan.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>


int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto window = glfwCreateWindow(800, 600, "title", nullptr, nullptr);

	if (!window)
	{
		return -1;
	}

	glfwMakeContextCurrent(window);

	Mulcan::initialize();
	glfwCreateWindowSurface();

	return 0;
}