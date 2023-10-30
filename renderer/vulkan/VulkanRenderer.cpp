#include "VulkanRenderer.hpp"

using namespace renderer;

VulkanRenderer::VulkanRenderer() {}
VulkanRenderer::~VulkanRenderer() {}

bool VulkanRenderer::Init() { return true; }
void VulkanRenderer::Release(){}

void VulkanRenderer::CreateInstance() {
	SDL_Window* window = udon::WsiWindow::GetInstance()->GetWindow();
    if (!window) {
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
    ++extensionsCount;
#endif


    std::vector<VkLayerProperties> enumLayers;
    uint32_t enumLayerCount;
    vkEnumerateInstanceLayerProperties(&enumLayerCount, enumLayers.data());
    enumLayers.resize(enumLayerCount);
    vkEnumerateInstanceLayerProperties(&enumLayerCount, enumLayers.data());

    //validation layers
    std::vector<const char*> layers;
    for (auto& enumLayer : enumLayers) {
        if (strcmp(enumLayer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }
    }

    info.setPpEnabledExtensionNames(extensionNames.data())
        .setPEnabledLayerNames(layers)
        .setEnabledExtensionCount(extensionsCount)
        .setEnabledLayerCount(layers.size());

#ifdef DEBUG
    std::cout << "Added Extensions:\n";
    for (auto& extension : extensionNames) std::cout << extension << std::endl;
    std::cout << "Added Layers:\n";
    for (auto& layer : layers) std::cout << layer << std::endl;
#endif

    _VkInstance = vk::createInstance(info);
}

void VulkanRenderer::CreatePhyDevice() {


}

