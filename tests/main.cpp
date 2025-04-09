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

    Mulcan::initialize(window);

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

    auto layout = Mulcan::buildPipelineLayout({}, 0, 0, VK_NULL_HANDLE);

    VkVertexInputBindingDescription vertex_input_binding{};
    vertex_input_binding.binding = 0;
    vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_binding.stride = sizeof(Mulcan::Vertex);

    std::array<VkVertexInputAttributeDescription, 2> input_attributes;

    // position binding
    input_attributes[0].binding = 0;
    input_attributes[0].location = 0;
    input_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[0].offset = offsetof(Mulcan::Vertex, Mulcan::Vertex::position);

    // color bindigs
    input_attributes[1].binding = 0;
    input_attributes[1].location = 1;
    input_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    input_attributes[1].offset = offsetof(Mulcan::Vertex, Mulcan::Vertex::color);

    Mulcan::NewPipelineDescription info{};

    info.vertex_shader = vert;
    info.fragment_shader = frag;
    info.renderpass = Mulcan::getMainPass();
    info.pipeline_layout = layout;
    info.input_attributes = input_attributes;
    info.binding_description = vertex_input_binding;

    auto p = Mulcan::buildPipeline(info);

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