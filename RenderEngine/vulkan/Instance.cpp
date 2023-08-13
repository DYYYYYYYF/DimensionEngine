#include "Instance.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <iostream>
#include <vector>

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

    // Enum all extensions
    std::vector<vk::ExtensionProperties> enumInstanceExtensions;
    uint32_t enumExtensionsCount;
    vk::Result res = vk::enumerateInstanceExtensionProperties(nullptr, &enumExtensionsCount, nullptr);
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
    std::vector<const char*> extensionNames;
    unsigned int extensionsCount;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionsCount, extensionNames.data());
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
        
    return vk::createInstance(info);
}
