#pragma once
#include <sys/wait.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

namespace VkCore{
class VkContext{
public:
    VkContext();
    virtual ~VkContext(){};

    bool Init();
    void Run();
    void Close();

    private:

};
}


