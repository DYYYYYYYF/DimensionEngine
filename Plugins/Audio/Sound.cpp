#include "Sound.hpp"
#include "sndfile.h"
#include "Core/EngineLogger.hpp"

Sound::Sound() {
	Buffer = NULL;
	Source = NULL;
}

Sound::~Sound() {
	Stop();
	alDeleteSources(1, &Source);
	alDeleteBuffers(1, &Buffer);
}

bool Sound::Load(const std::string& filename) {
	// 使用 libsndfile 或其他库加载音频文件数据
	SNDFILE* file;
	SF_INFO sfInfo;
	file = sf_open(filename.c_str(), SFM_READ, &sfInfo);
	if (!file) {
		GLOG(Log::eError, "Failed to load sound file: %s.", filename.c_str());
		return false;
	}

	// 获取音频数据
	short* Samples = new short[sfInfo.frames * sfInfo.channels];
	if (sf_read_short(file, Samples, sfInfo.frames * sfInfo.channels) == 0) {
		GLOG(Log::eError, "Failed to load sound file: %s. Can not read short data.", filename.c_str());
		sf_close(file);
		delete[] Samples;
		return false;
	}
	sf_close(file);

	// 创建 OpenAL 音频缓冲区
	alGenBuffers(1, &Buffer);

	// 单声道
	ALenum AlFormat;
	if (sfInfo.channels == 1) {
		AlFormat = AL_FORMAT_MONO16;
	}
	// 立体声道
	else if (sfInfo.channels == 2) {
		AlFormat = AL_FORMAT_STEREO16;
	}
	else {
		GLOG(Log::eWarn, "Sound load warning: %s sound's channels if not 1(single) or 2(stereo). Default use 1(single) mode AL_FORMAT_MONO16.", filename.c_str());
		AlFormat = AL_FORMAT_MONO16;
	}

	alBufferData(Buffer, AlFormat, Samples, ALsizei(sfInfo.frames * sfInfo.channels * sizeof(short)), sfInfo.samplerate);
	delete[] Samples;

	// 创建音频源
	alGenSources(1, &Source);
	alSourcei(Source, AL_BUFFER, Buffer);

	return true;
}

void Sound::Play() {
	alSourcePlay(Source);
}

void Sound::Stop() {
	alSourceStop(Source);
}
