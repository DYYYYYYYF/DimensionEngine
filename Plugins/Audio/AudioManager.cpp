#include "AudioManager.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

AudioManager::AudioManager() {
	
}

AudioManager::~AudioManager() {
	Shutdown();
}

bool AudioManager::Initialize() {

	return true;
}

void AudioManager::Shutdown() {
	for (auto& s : Sounds) {
		DeleteObject(s.second);
	}

	Sounds.clear();
}

bool AudioManager::LoadSound(const std::string& filename) {
	Sound* NewSound = NewObject<Sound>();
	if (!NewSound->Load(filename)) {
		DeleteObject(NewSound);
		NewSound = nullptr;
		return false;
	}

	Sounds[filename] = NewSound;
	return true;
}

void AudioManager::PlaySound(const std::string& filename) {
	if (Sounds.find(filename) == Sounds.end()) {
		GLOG(Log::eWarn, "Can not found audio asset %s. Loading...", filename.c_str());
		if (!LoadSound(filename)) {
			GLOG(Log::eWarn, "Load audio failed.");
			return;
		}

		// 如果仍然找不到则加载失败。
		if (Sounds.find(filename) == Sounds.end()) {
			return;
		}
	}

	Sounds[filename]->Play();
}

void AudioManager::StopSound(const std::string& filename) {
	if (Sounds.find(filename) == Sounds.end()) {
		GLOG(Log::eWarn, "Can not found audio asset %s.", filename.c_str());
		return;
	}

	Sounds[filename]->Stop();
}
