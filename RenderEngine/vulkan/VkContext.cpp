#include "VkContext.hpp"
#include "Device.hpp"

using namespace udon;
using namespace VkCore;

VkContext::~VkContext(){
    Release();
}

VkContext::VkContext(){
    _Instance = nullptr;
    _Device = nullptr;
    _Surface = nullptr;
}

bool VkContext::InitContext(){
    if (!CreateInstance()) return false;
    if (!CreatePhysicalDevice()) return false; 
    if (!CreateSurface()) return false;
    if (!CreateDevice()) return false;

    return true;
}

// Instance
bool VkContext::CreateInstance(){

    if (!InitWindow()){
        DEBUG("NULL WINDOW.");
        return false;
    }

    //Instance
    _Instance = new Instance();
    CHECK(_Instance);
    _VkInstance = _Instance->CreateInstance(_Window);
    CHECK(_VkInstance);

    return true;
}

bool VkContext::InitWindow(){
    _Window =  WsiWindow::GetInstance()->GetWindow();
    return true;
}

bool VkContext::CreatePhysicalDevice(){
    if (!_Device) _Device = new Device();
    CHECK(_Device);
    
    _VkPhyDevice = _Device->CreatePhysicalDeivce(_VkInstance);
    CHECK(_VkPhyDevice);

    return true;
}

bool VkContext::CreateDevice(){
    if (!_Device) _Device = new Device();
    CHECK(_Device);

    _VkDevice = _Device->CreateDevice(_VkSurface);
    CHECK(_VkDevice);

    return true;
}

bool VkContext::CreateSurface(){

    //Surface
    _Surface = new Surface(_VkInstance, _Window);
    CHECK(_Surface);
    _VkSurface = _Surface->CreateSurface();
    CHECK(_VkSurface);

    return true;
}

void VkContext::Release(){

}

