#include <iostream>
#include "mulcan.hpp"
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;

struct Object
{
    int id;
    std::vector<Mulcan::Vertex> vertices;
    std::vector<uint32_t> indices;

    glm::vec3 pos;
    glm::vec3 scale;
    glm::vec3 rotation;
};

glm::mat4 CreateModelMatrix(const glm::vec3 &pPos, const glm::vec3 &pScale, const glm::vec3 &pRotation)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pPos);
    model = glm::rotate(model, pRotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, pRotation.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, pRotation.z, glm::vec3(0, 0, 1));
    model = glm::scale(model, pScale);
    return model;
}

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
        WIDTH,
        HEIGHT,
        window_flags);

    Mulcan::initialize(window, WIDTH, HEIGHT);

    Mulcan::SpawnSampleCube();

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

        Mulcan::beginFrame();

        Mulcan::RenderWorldSystem();

        Mulcan::endFrame();
    }

    Mulcan::shutdown();

    SDL_DestroyWindow(window);
}