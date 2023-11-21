#pragma once
#include <GLFW/glfw3.h>
#include "utils/EngineUtils.hpp"

namespace udon {
class WsiWindow{
public:
    static GLFWwindow* GetWindow();
    static WsiWindow* GetInstance();
    static void DestoryWindow();
    virtual ~WsiWindow();

    static void SetWidth(int width) { _Width = width; }
    static int GetWidth() { return _Width; }

    static void SetHeight(int height) { _Height = height; }
    static int GetHeith() { return _Height; }

    static void GetWindowSize(int& w, int& h);
    static void SetWidthHeight(int width, int height) {
        _Width = width;
        _Height = height;
    }

    static void SetAspect(float aspect) { _Aspect = aspect; }
    static float GetAspect() { return _Aspect; }

private:
    WsiWindow();
    WsiWindow(int width);
    WsiWindow(int height, int width);
    static bool InitWindow();

private:
    static float _Aspect;
    static int _Height;
    static int _Width;
    static GLFWwindow* _GLFWWindow;

    static WsiWindow* _WindowObj;
};
}
