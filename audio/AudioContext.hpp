#pragma once

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/alut.h>
#include <OpenAL/OpenAL.h>

class AudioContext{
public:
    AudioContext();
    virtual ~AudioContext();

    void InitAudio();

private:
    ALCdevice* m_pAudioDevice;
    ALCcontext* m_pAudioContext;

};

