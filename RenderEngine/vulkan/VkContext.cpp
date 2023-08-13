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
}

bool VkContext::InitContext(){
    if (!CreateInstance()) return false;

    return true;
}

// Instance
bool VkContext::CreateInstance(){

    if (!InitWindow()){
        DEBUG("NULL WINDOW.");
        return false;
    }

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
    
    _VkPhyDevice = _Device->CreatePhysicalDeivce();
    CHECK(_VkPhyDevice);

    return true;
}

bool VkContext::CreateDevice(){
    if (_Device) _Device = new Device();
    CHECK(_Device);

    _VkDevice = _Device->CreateDevice();
    CHECK(_VkDevice);

    return true;
}

void VkContext::Release(){

}

