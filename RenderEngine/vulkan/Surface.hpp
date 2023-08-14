#pragma once
#include <vulkan/vulkan.hpp>
#include "../application/Window.hpp"
#include "vulkan/vulkan_handles.hpp"

class Surface{
    Surface();
    Surface(SDL_Window* window);
    Surface(vk::Instance instance, SDL_Window* window);
    virtual ~Surface();

    virtual vk::SurfaceKHR CreateSurface();

private:
    SDL_Window* _Window;
    vk::Instance _VkInstance;

};
