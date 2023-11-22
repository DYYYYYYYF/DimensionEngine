#pragma once

#include <al.h>
#include <alc.h>

class AudioContext{
public:
    AudioContext();
    virtual ~AudioContext();

    void InitAudio();

private:
    ALCdevice* m_pAudioDevice;
    ALCcontext* m_pAudioContext;

};

