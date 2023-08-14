#include "Surface.hpp"

Surface::Surface(){
    _Window = udon::WsiWindow::GetInstance()->GetWindow();
}

Surface::Surface(SDL_Window* window) : _Window(window){};
Surface::Surface(vk::Instance instance, SDL_Window* window) : _VkInstance(instance), _Window(window){};


vk::SurfaceKHR Surface::CreateSurface(){
    vk::SurfaceKHR surface;
    // SDL_Vulkan_CreateSurface(_Window, _VkInstance, &surface);


    return surface;
}
