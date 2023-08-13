#include "VkContext.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <cstdlib>
#include <vector>

using namespace udon;
using namespace VkCore;

VkContext::~VkContext(){
    Release();
}

VkContext::VkContext(){
    _VkInstance = nullptr;
}


bool VkContext::InitInstance(){
    return InitVulkan() && InitWindow();
}

bool VkContext::InitWindow(){
    _Window =  WsiWindow::GetInstance()->GetWindow();
    return true;
}

bool VkContext::InitVulkan(){
    // Enum all extensions
    std::vector<vk::ExtensionProperties> enumInstanceExtensions;
    uint32_t enumExtensionsCount;
    vk::Result res = vk::enumerateInstanceExtensionProperties(nullptr, &enumExtensionsCount, enumInstanceExtensions.data());
    enumInstanceExtensions.resize(enumExtensionsCount);
    res = vk::enumerateInstanceExtensionProperties(nullptr, &enumExtensionsCount, enumInstanceExtensions.data());
    if (res != vk::Result::eSuccess) {
        std::cout << "Enum instance extensions:\n";
        for (auto& extension : enumInstanceExtensions){
            std::cout << "\t" << extension.extensionName << std::endl;
        }
    }

    vk::InstanceCreateInfo info;

    // Get SDL instance extensions
    const char** extensionNames;
    unsigned int extensionsCount;
    SDL_Vulkan_GetInstanceExtensions(_Window, &extensionsCount, extensionNames);

    #ifdef __APPLE_
        // MacOS requirment
        extensionNames[extensionsCount] = "VK_KHR_get_physical_device_properties2";
    #endif

    //validation layers
    std::array<const char*, 1> layers{"VK_LAYER_KHRONOS_validation"};

    info.setPpEnabledExtensionNames(extensionNames)
        .setPpEnabledLayerNames(layers.data())
        .setEnabledExtensionCount(++extensionsCount)
        .setEnabledLayerCount(layers.size());
        
    _VkInstance = vk::createInstance(info);
    CHECK(_VkInstance);

    return  true;
}

void VkContext::Release(){

}

