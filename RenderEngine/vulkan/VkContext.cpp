#include "VkContext.hpp"
#include "CmdBuffer.hpp"
#include "Device.hpp"
#include "RenderPass.hpp"
#include "SwapChain.hpp"
#include "interface/IDevice.hpp"

using namespace udon;
using namespace VkCore;

VkContext::~VkContext(){
    Release();
}

VkContext::VkContext(){
    _Instance = nullptr;
    _Device = nullptr;
    _Surface = nullptr;
    _Swapchain = nullptr;
    _RenderPass = nullptr;
    _ICmdBuffer = nullptr;
}

bool VkContext::InitContext(){
    if (!CreateInstance()) return false;
    if (!CreatePhysicalDevice()) return false; 
    if (!CreateSurface()) return false;
    if (!CreateDevice()) return false;

    _QueueFamily = _Device->GetQueueFamily();
    if (!_Queue.InitQueue(_VkDevice, _QueueFamily)) return false;

    if (!CreateSwapchain()) return false;
    _Swapchain->GetImages(_Images);
    _Swapchain->GetImageViews(_Images, _ImageViews);

    if (!CreateRenderPass()) return false;
    if (!AllocateCmdBuffer()) return false;

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

    _VkDevice = _Device->CreateDevice(_SurfaceKHR);
    CHECK(_VkDevice);

    return true;
}

bool VkContext::CreateSurface(){
    _Surface = new Surface(_VkInstance, _Window);
    CHECK(_Surface);
    _SurfaceKHR = _Surface->CreateSurface();
    CHECK(_SurfaceKHR);

    return true;
}

bool VkContext::CreateSwapchain(){
    _Swapchain = new SwapChain(_SurfaceKHR, _QueueFamily);
    CHECK(_Swapchain);
    _Swapchain->SetDevice(_VkDevice);
    _SwapchainKHR = _Swapchain->CreateSwapchain(_VkPhyDevice);
    CHECK(_SwapchainKHR);

    return true;
}

bool VkContext::CreateRenderPass(){
    _RenderPass = new RenderPass();
    CHECK(_RenderPass);
    _RenderPass->SetDevice(_VkDevice);
    _RenderPass->SetPhyDevice(_VkPhyDevice);
    _RenderPass->SetSupportInfo(_Swapchain->GetSwapchainSupport());

    _VkRenderPass = _RenderPass->CreateRenderPass();
    CHECK(_VkRenderPass);
        
    return true;
}

bool VkContext::AllocateCmdBuffer(){
    _ICmdBuffer = new CommandBuf();
    CHECK(_ICmdBuffer);
    _CmdPool = _ICmdBuffer->CreateCmdPool(_Device);
    CHECK(_CmdPool);
    _CmdBuffer = _ICmdBuffer->AllocateCmdBuffer(_Device);
    CHECK(_CmdBuffer);

    return true;
}

void VkContext::Release(){

}

