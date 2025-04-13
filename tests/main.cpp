#include <iostream>
#include "mulcan.hpp"
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

const std::vector<Mulcan::Vertex> vertices = {
    // Position                 // Color                // TexCoord           // Normal
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},   // 0: v1 (red)
    {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 1: v2 (green)
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},     // 2: v3 (blue)
    {{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},    // 3: v4 (yellow)
    {{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 4: v5 (magenta)
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}, // 5: v6 (cyan)
    {{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},    // 6: v7 (white)
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}    // 7: v8 (gray)
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

struct GPUCameraData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

int main()
{

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    auto window = SDL_CreateWindow(
        "Vulkan Engine",
        800,
        600,
        window_flags);

    Mulcan::initialize(window, 1920, 1080);

    auto device = Mulcan::getDevice();
    auto alloc = Mulcan::getAllocator();
    auto renderpass = Mulcan::getMainPass();

    Mulcan::Pipeline pipeline{device, renderpass};
    Mulcan::DescriptorManager descriptorManager{alloc, device};

    auto vb = Mulcan::createTransferBuffer<Mulcan::Vertex>(vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    auto ib = Mulcan::createTransferBuffer<uint32_t>(indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    Mulcan::runTransferBufferCommand();

    VkDescriptorSetLayoutBinding bindingOne{};
    bindingOne.binding = 0;
    bindingOne.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindingOne.descriptorCount = 1;
    bindingOne.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    descriptorManager.CreateDescriptorLayout(1, {bindingOne});
    auto a = descriptorManager.CreateUniformBuffer(sizeof(GPUCameraData));
    descriptorManager.CreateDescriptorSet(1, a.buffer, sizeof(GPUCameraData));

    VkPushConstantRange pushConsant{};
    pushConsant.size = sizeof(Mulcan::MeshPushConstants);
    pushConsant.offset = 0;
    pushConsant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    pipeline.CreatePipelineLayout({pushConsant}, {descriptorManager.GetDescriptorLayout(1)});
    pipeline.CreatePipeline("./shaders/cube-v.spv", "./shaders/cube-f.spv");

    bool fullscreen = false;
    bool quit = false;
    SDL_Event event;

    while (!quit)
    {
        bool eventHandledThisFrame = false;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
            {
                quit = true;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F)
            {
                fullscreen = !fullscreen;

                SDL_SetWindowFullscreen(window, fullscreen);
            }

            if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event.type == SDL_EVENT_WINDOW_METAL_VIEW_RESIZED)
            {

                int height, width;
                auto res = SDL_GetWindowSize(window, &width, &height);
                if (!res)
                {
                    std::cout << "could not get window size" << std::endl;
                }

                Mulcan::recreateSwapchain(width, height);
                eventHandledThisFrame = true;
            }
        }

        if (eventHandledThisFrame)
        {
            continue;
        }

        static int framenumber = 0;
        Mulcan::beginFrame();

        VkDeviceSize offsets[1]{0};

        vkCmdBindPipeline(Mulcan::getCurrCommand(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipeline());

        vkCmdBindVertexBuffers(Mulcan::getCurrCommand(), 0, 1, &vb, offsets);
        vkCmdBindIndexBuffer(Mulcan::getCurrCommand(), ib, 0, VK_INDEX_TYPE_UINT32);

        // camera view
        glm::vec3 camPos = {0.f, -0.f, -5.f};

        glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
        // camera projection
        glm::mat4 projection = glm::perspective(glm::radians(70.f), 800.f / 600.f, 0.1f, 200.0f);
        projection[1][1] *= -1;
        // model rotation
        glm::mat4 model = glm::rotate(glm::mat4{1.0f}, glm::radians(framenumber * 0.4f), glm::vec3(0, 1, 0));

        // calculate final mesh matrix
        glm::mat4 mesh_matrix = projection * view * model;

        Mulcan::MeshPushConstants constants;
        constants.render_matrix = model;

        // // fill a GPU camera data struct
        GPUCameraData camData;
        camData.proj = projection;
        camData.view = view;
        camData.viewproj = projection * view;

        descriptorManager.UpdateBuffer<GPUCameraData>(&a, camData);

        vkCmdBindDescriptorSets(Mulcan::getCurrCommand(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipelineLayout(), 0, 1, &descriptorManager.GetDescriptorSet(1), 0, nullptr);
        // calculate final mesh matrix
        vkCmdPushConstants(Mulcan::getCurrCommand(), pipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mulcan::MeshPushConstants), &constants);

        vkCmdDrawIndexed(Mulcan::getCurrCommand(), indices.size(), 1, 0, 0, 0);

        Mulcan::endFrame();
        framenumber++;
    }

    pipeline.DestroyPipeline();
    Mulcan::addDestroyBuffer(vb);
    Mulcan::addDestroyBuffer(ib);
    Mulcan::shutdown();

    SDL_DestroyWindow(window);
}