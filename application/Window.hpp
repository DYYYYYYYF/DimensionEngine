#pragma once
#include <GLFW/glfw3.h>
#include "utils/EngineUtils.hpp"

namespace udon {
class WsiWindow{
public:
    static GLFWwindow* GetWindow();
    static WsiWindow* GetInstance();
    static void GetWindowSize(int& w, int& h);
    static void DestoryWindow();
    virtual ~WsiWindow();

private:
    WsiWindow(){}
    WsiWindow(int height, int width);
    static bool InitWindow();

private:
    static int _Height;
    static int _Width;
    static GLFWwindow* _GLFWWindow;

    static WsiWindow* _WindowObj;
};
}
