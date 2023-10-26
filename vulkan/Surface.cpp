#include "Surface.hpp"

using namespace VkCore;

Surface::Surface(){_Window = udon::WsiWindow::GetInstance()->GetWindow();}
Surface::Surface(SDL_Window* window) : _Window(window){};
Surface::Surface(vk::Instance instance, SDL_Window* window) : _VkInstance(instance), _Window(window){};


vk::SurfaceKHR Surface::CreateSurface(){
    CHECK(_Window);
    CHECK(_VkInstance);

    VkSurfaceKHR surface;

    if (!SDL_Vulkan_CreateSurface(_Window, _VkInstance, &surface)){
        INFO("Create surface failed.");
        return nullptr;
    }

    CHECK(surface);
    _VkSurface = vk::SurfaceKHR(surface);
    CHECK(_VkSurface);

    return _VkSurface;
}

Surface::~Surface(){
    _VkInstance.destroySurfaceKHR(_VkSurface);
}
