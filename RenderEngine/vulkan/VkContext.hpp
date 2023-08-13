#pragma once
#include "interface/IContext.hpp"
#include "Instance.hpp"
#include "Device.hpp"

namespace VkCore{
class VkContext : public IContext{
public:
    VkContext();
    virtual ~VkContext();
    virtual bool InitContext();
    virtual void Release();

public:
    bool CreateInstance();
    bool CreatePhysicalDevice();
    bool CreateDevice();

    vk::Instance GetInstance() const {return _VkInstance;} 
    SDL_Window* GetWindow() const {return _Window;}

private:
    bool InitWindow();

private:
    Instance* _Instance;
    Device* _Device;

};
}


