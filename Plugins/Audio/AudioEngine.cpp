#include "AudioEngine.hpp"
#include "AudioManager.hpp"
#include "Core/DMemory.hpp"

AudioEngine::AudioEngine() {
	aDevice = nullptr;
	aContext = nullptr;
	Manager = nullptr;
}

AudioEngine::~AudioEngine() {
	Shutdown();
}

bool AudioEngine::Initalize() {
	// 创建 OpenAL 设备
	aDevice = alcOpenDevice(nullptr); // nullptr 表示使用默认设备
	if (!aDevice) {
		GLOG(Log::eError, "Failed to open OpenAL device");
		return false;
	}

	// 创建 OpenAL 上下文
	aContext = alcCreateContext(aDevice, nullptr);
	if (!aContext) {
		GLOG(Log::eError, "Failed to create OpenAL context");
		alcCloseDevice(aDevice);
		return false;
	}

	// 设置当前上下文
	if (!alcMakeContextCurrent(aContext)) {
		GLOG(Log::eError, "Failed to make OpenAL context current");
		alcDestroyContext(aContext);
		alcCloseDevice(aDevice);
		return false;
	}

	Manager = NewObject<AudioManager>();
	return Manager->Initialize();
}

void AudioEngine::Update() {

}

void AudioEngine::Shutdown() {
	if (Manager) {
		Manager->Shutdown();
		DeleteObject(Manager);
		Manager = nullptr;
	}

	if (aContext) {
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(aContext);
		aContext = nullptr;
	}

	if (aDevice) {
		alcCloseDevice(aDevice);
		aDevice = nullptr;
	}
}