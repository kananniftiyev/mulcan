#include <iostream>
#include "mulcan.hpp"
#include <SDL3/SDL.h>

constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;

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
    Mulcan::setVsync(false);

    Mulcan::spawnSampleCube();

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

        Mulcan::renderWorldSystem();

        Mulcan::endFrame();
    }

    Mulcan::shutdown();

    SDL_DestroyWindow(window);
}