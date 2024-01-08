#pragma once
#include "Window.hpp"
#include "EngineUtils.hpp"
#include "../engine/Scene.hpp"

using namespace renderer;

class Application{
public:
    Application();
    Application(ConfigFile* config);
    virtual ~Application();

    bool Init();
    void Run();
    void Close();

public:
    void SetIsLimitFPS(bool is_limit) { m_bLimitFPS = is_limit; }
    bool GetIsLimitFPS() const { return m_bLimitFPS; }

    void SetLimitFPS(int fps) { m_nLimitFPS = fps; }
    int GetLimitFPS() const { return m_nLimitFPS; }
    int GetCurrentFPS(double delta_time) const { return (int)(1.0 / delta_time); }

private:
    GLFWwindow* _window;
    Scene* _Scene;
    ConfigFile* _ConfigFile;
    int _FrameCount = 0;

#ifdef _WIN32
    LARGE_INTEGER _Time, _Freq;
#elif __APPLE__

#endif

    int m_nFPS, m_nLimitFPS;
    bool m_bLimitFPS;
};
