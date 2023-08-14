#pragma once
#include "interface/IInstance.hpp"
#include "vulkan/vulkan_handles.hpp"

namespace VkCore{
class Instance : public IInstance{
public:
    Instance();
    virtual ~Instance();

    virtual vk::Instance CreateInstance();
    virtual vk::Instance CreateInstance(SDL_Window* window);

};
}
