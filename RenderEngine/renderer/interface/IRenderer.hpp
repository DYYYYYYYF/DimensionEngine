#pragma once
#include "../../vulkan/VkContext.hpp"

using namespace VkCore;

class IRenderer{
public:
    IRenderer(){};
    virtual ~IRenderer(){};
    virtual bool Init() = 0;

protected:
    VkContext* _Context;

};
