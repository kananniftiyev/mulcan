#include <iostream>
#include "mulcan.hpp"
#include <GLFW/glfw3.h>

const std::vector<Mulcan::Vertex> vertices = {
    // Front face
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}}, // Bottom-left
    {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},  // Bottom-right
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},   // Top-right
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},  // Top-left
    // Back face
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // Bottom-left
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},  // Bottom-right
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},   // Top-right
    {{-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}   // Top-left
};

const std::vector<uint32_t> indices = {
    // Front face
    0, 1, 2, 0, 2, 3,

    // Back face
    4, 5, 6, 4, 6, 7,

    // Left face
    4, 0, 3, 4, 3, 7,

    // Right face
    1, 5, 6, 1, 6, 2,

    // Top face
    3, 2, 6, 3, 6, 7,

    // Bottom face
    4, 5, 1, 4, 1, 0};

int main()
{
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

    VkShaderModule vert, frag;

    auto res_v = Mulcan::loadShaderModule("shaders/cube-v.spv", &vert);
    ;
    if (!res_v)
    {
        std::cout << "Could not load vertex shader\n";
        abort();
    }

    res_v = Mulcan::loadShaderModule("shaders/cube-f.spv", &frag);

    if (!res_v)
    {
        std::cout << "Could not load frag shader\n";
        abort();
    }

    std::cout << "Loaded shaders\n";

    auto vb = Mulcan::createTransferBuffer<Mulcan::Vertex>(vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    auto ib = Mulcan::createTransferBuffer<uint32_t>(indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    std::cout << "create transfer buffers success\n";

    Mulcan::runTransferBufferCommand();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Specifies the shader stage
    pushConstantRange.offset = 0;                              // Start at the beginning of the push constant range
    pushConstantRange.size = sizeof(float) * 16;               // Assuming a 4x4 matrix, 16 floats

    auto layout = Mulcan::buildPipelineLayout({}, 0, 0, VK_NULL_HANDLE);

    Mulcan::NewPipelineDescription info{};

    info.vertex_shader = vert;
    info.fragment_shader = frag;
    info.renderpass = Mulcan::getMainPass();
    info.pipeline_layout = layout;

    Mulcan::buildPipeline(info);

    while (!glfwWindowShouldClose(window))
    {
        Mulcan::beginFrame();

        VkDeviceSize offsets[1]{0};

        vkCmdBindPipeline(Mulcan::getCurrCommand(), VK_PIPELINE_BIND_POINT_GRAPHICS, p);

        vkCmdBindVertexBuffers(Mulcan::getCurrCommand(), 0, 1, &vb, offsets);
        vkCmdBindIndexBuffer(Mulcan::getCurrCommand(), ib, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(Mulcan::getCurrCommand(), indices.size(), 1, 0, 0, 0);

        Mulcan::endFrame();

        glfwPollEvents();
    }

    Mulcan::shutdown();
}