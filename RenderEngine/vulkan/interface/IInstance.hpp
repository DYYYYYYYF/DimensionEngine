#pragma once
#include <vulkan/vulkan.hpp>
#include "../../engine/EngineLogger.hpp"
#include "../../application/Window.hpp"

class IInstance{
public:
    IInstance(){}
    virtual ~IInstance(){};

    virtual vk::Instance CreateInstance() = 0;

protected:
    vk::Instance _Intance;
};
