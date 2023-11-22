#include "AudioContext.hpp"

AudioContext::AudioContext() : m_pAudioDevice(nullptr), m_pAudioContext(nullptr){

}

AudioContext::~AudioContext(){

}

void AudioContext::InitAudio(){

    m_pAudioDevice = alcOpenDevice(nullptr);
    m_pAudioContext = alcCreateContext(m_pAudioDevice, nullptr);
    alcMakeContextCurrent(m_pAudioContext);

    // Create audio
    ALuint source;
    alGenSources(1, &source);

    // Load audio
    ALuint buffer;
    alGenBuffers(1, &buffer);
    ALsizei size, frequency;
    ALenum format;
    ALvoid* data;

    ALbyte file[] = "sound.wav";
    
    alBufferData(buffer, format, data, size, frequency);

    // Bind audio
    alSourcei(source, AL_BUFFER, buffer);

    // Play
    alSourcePlay(source);
}
