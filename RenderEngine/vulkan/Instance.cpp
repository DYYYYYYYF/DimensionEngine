#include "Instance.hpp"
#include <iostream>
#include <vector>

using namespace VkCore; 

Instance::Instance(){

}

Instance::~Instance(){

}

vk::Instance Instance::CreateInstance(){
    SDL_Window* window = udon::WsiWindow::GetInstance()->GetWindow();
    return CreateInstance(window);
}

vk::Instance Instance::CreateInstance(SDL_Window* window){
    if (!window){
        DEBUG("Error Window.");
        exit(-1);
    }

    vk::InstanceCreateInfo info;

    // Get SDL instance extensions
    std::vector<const char*> extensionNames;
    unsigned int extensionsCount;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionsCount, nullptr);
    extensionNames.resize(extensionsCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionsCount, extensionNames.data());

    #ifdef __APPLE__
        // MacOS requirment
        extensionNames.push_back("VK_KHR_get_physical_device_properties2");
    #endif

    //validation layers
    std::array<const char*, 1> layers{"VK_LAYER_KHRONOS_validation"};

    info.setPpEnabledExtensionNames(extensionNames.data())
        .setPEnabledLayerNames(layers)
        .setEnabledExtensionCount(++extensionsCount)
        .setEnabledLayerCount(layers.size());

    std::cout << "Added Extensions:\n";
    for(auto& extension : extensionNames) std::cout << extension << std::endl;
    std::cout << "Added Layers:\n";
    for(auto& layer : layers) std::cout << layer << std::endl;
        
    return vk::createInstance(info);
}
