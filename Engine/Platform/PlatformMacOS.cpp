#include "Platform.hpp"

#ifdef DPLATFORM_MACOS

#include "Core/DThread.hpp"
#include "Core/DMutex.hpp"
#include "Renderer/Vulkan/VulkanPlatform.hpp"

bool Platform::PlatformStartup(SPlatformState* platform_state, const char* application_name,
	int x, int y, int width, int height){

  return true;
}

void Platform::PlatformShutdown(SPlatformState* platform_state){

}

bool Platform::PlatformPumpMessage(SPlatformState* platform_state){

  return true;
}

void* Platform::PlatformAllocate(size_t size, bool aligned){

  return nullptr;
}

void Platform::PlatformFree(void* block, bool aligned){

}

void* Platform::PlatformZeroMemory(void* block, size_t size){

  return nullptr;
}

void* Platform::PlatformCopyMemory(void* dst, const void* src, size_t size){

  return nullptr;
}

void* Platform::PlatformSetMemory(void* dst, int val, size_t size){

  return nullptr;
}

void Platform::PlatformConsoleWrite(const char* message, unsigned char color){

}

void Platform::PlatformConsoleWriteError(const char* message, unsigned char color){

}

double Platform::PlatformGetAbsoluteTime(){

  return 0.0;
}

void Platform::PlatformSleep(size_t ms){

}

int Platform::GetProcessorCount(){

  return 8;
}

// Vulkan
bool PlatformCreateVulkanSurface(SPlatformState* plat_state, VulkanContext* context) {

  return true;
}

void GetPlatformRequiredExtensionNames(std::vector<const char*>& array){

}

// NOTE: Begin Threads
bool Thread::Create(PFN_thread_start start_func, void* params, bool auto_detach) {

	return true;
}

void Thread::Destroy() {

}

void Thread::Detach() {
	if (InternalData == nullptr) {
		return;
	}

}

void Thread::Cancel() {
	if (InternalData == nullptr) {
		return;
	}

}

bool Thread::IsActive() const {
	if (InternalData == nullptr) {
		return false;
	}

	return false;
}

void Thread::Sleep(size_t ms) {
	Platform::PlatformSleep(ms); 
}

size_t Thread::GetThreadID() {

  return 0;
}
// NOTE: End Threads

// NOTE: Begin mutexs
bool Mutex::Create() {

	return true;
}

void Mutex::Destroy() {
	if (InternalData == nullptr) {
		return;
	}

}

bool Mutex::Lock() {
	if (InternalData == nullptr) {
		return false;
	}


	return true;
}

bool Mutex::UnLock() {
	if (InternalData == nullptr) {
		return false;
	}

  return false;
}

// NOTE: End mutexs.


#endif
