#include <iostream>
#include "mulcan.hpp"
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

const std::vector<Mulcan::Vertex> vertices = {
    // Position                     // Color (using arbitrary colors)
    {{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}},   // 0: v1 (red)
    {{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}},  // 1: v2 (green)
    {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},    // 2: v3 (blue)
    {{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},   // 3: v4 (yellow)
    {{-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}},  // 4: v5 (magenta)
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}}, // 5: v6 (cyan)
    {{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},   // 6: v7 (white)
    {{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}   // 7: v8 (gray)
};

// Indices for the cube faces
const std::vector<uint32_t> indices = {
    // Top face (Y+)
    0, 4, 6,
    0, 6, 2,

    // Front face (Z+)
    2, 6, 7,
    2, 7, 3,

    // Left face (X-)
    6, 4, 5,
    6, 5, 7,

    // Bottom face (Y-)
    3, 7, 5,
    3, 5, 1,

    // Right face (X+)
    0, 2, 3,
    0, 3, 1,

    // Back face (Z-)
    0, 1, 5,
    0, 5, 4};

int main()
{

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    auto window = SDL_CreateWindow(
        "Vulkan Engine",
        800,
        600,
        window_flags);

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

    Mulcan::runTransferBufferCommand();

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // Add this range for the vertex shader
    pushConstantRange.offset = 0;                               // Offset into the push constant space
    pushConstantRange.size = sizeof(Mulcan::MeshPushConstants); // Size of the push constants

    auto layout = Mulcan::buildPipelineLayout(pushConstantRange, 1, 0, VK_NULL_HANDLE);

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

    bool quit = false;
    SDL_Event event;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
            {
                quit = true;
            }

            // Handle other events here if necessary
        }
        static int framenumber = 0;
        Mulcan::beginFrame();

        VkDeviceSize offsets[1]{0};

        vkCmdBindPipeline(Mulcan::getCurrCommand(), VK_PIPELINE_BIND_POINT_GRAPHICS, p);

        vkCmdBindVertexBuffers(Mulcan::getCurrCommand(), 0, 1, &vb, offsets);
        vkCmdBindIndexBuffer(Mulcan::getCurrCommand(), ib, 0, VK_INDEX_TYPE_UINT32);

        glm::vec3 camPos = {0.f, 0.f, -5.f};

        glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
        // camera projection
        glm::mat4 projection = glm::perspective(glm::radians(70.f), 800.f / 600.f, 0.1f, 100.0f);
        projection[1][1] *= -1;
        // model rotation
        glm::mat4 model = glm::rotate(glm::mat4{1.0f}, glm::radians(framenumber * 0.4f), glm::vec3(0, 1, 0));

        // calculate final mesh matrix
        glm::mat4 mesh_matrix = projection * view * model;

        Mulcan::MeshPushConstants constants;
        constants.render_matrix = mesh_matrix;

        vkCmdPushConstants(Mulcan::getCurrCommand(), layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mulcan::MeshPushConstants), &constants);

        vkCmdDrawIndexed(Mulcan::getCurrCommand(), indices.size(), 1, 0, 0, 0);

        Mulcan::endFrame();
        framenumber++;
    }

    Mulcan::addDestroyBuffer(vb);
    Mulcan::addDestroyBuffer(ib);
    Mulcan::shutdown();

    SDL_DestroyWindow(window);
}