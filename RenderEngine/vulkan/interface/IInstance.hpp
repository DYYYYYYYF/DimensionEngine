#pragma once
#include <vulkan/vulkan.hpp>
#include "../../application/Window.hpp"

namespace VkCore{
class IInstance{
public:
    IInstance(){}
    virtual ~IInstance(){};

    virtual vk::Instance CreateInstance() = 0;

protected:
    vk::Instance _Intance = nullptr;
};
}
