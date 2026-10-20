#pragma once
#include "Containers/TMap.hpp"
#include <Al/al.h>
#include "Sound.hpp"
#include <string>
#include <unordered_map>

class Sound;

class AudioManager {
public:
	AudioManager();
	virtual ~AudioManager();

public:
	bool Initialize();
	void Shutdown();

	DLLEXPORT bool LoadSound(const std::string& filename);
	DLLEXPORT void PlaySound(const std::string& filename);
	DLLEXPORT void StopSound(const std::string& filename);

private:
	TMap<std::string, Sound*> Sounds;
};