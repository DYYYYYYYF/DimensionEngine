#include "VulkanBackend.hpp"

VulkanBackend::VulkanBackend() {

}

VulkanBackend::~VulkanBackend() {

}

bool VulkanBackend::Init(RendererBackendType type, struct SPlatformState* plat_state) {

	return true;
}

void VulkanBackend::Destroy() {

}

bool VulkanBackend::Initialize(const char* application_name, struct SPlatformState* plat_state) {

	return true;
}

void VulkanBackend::Shutdown() {

}

bool VulkanBackend::BeginFrame(double delta_time){

	return true;
}

bool VulkanBackend::EndFrame(double delta_time) {

	return true;
}

void VulkanBackend::Resize(unsigned short width, unsigned short height) {

}