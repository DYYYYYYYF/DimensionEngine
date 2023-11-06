#include"Window.hpp"

namespace udon {
    float scale_callback = 0.5;
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
        if (scale_callback + yoffset * 0.02 >= 0.1)
            scale_callback += yoffset * 0.02;
    }
}

using namespace udon;

int WsiWindow::_Height = 800;
int WsiWindow::_Width = 1200;
WsiWindow* WsiWindow::_WindowObj = nullptr;
GLFWwindow* WsiWindow::_GLFWWindow = nullptr;

WsiWindow::WsiWindow(int width, int height){
    _WindowObj = nullptr;
    _Height = height;
    _Width = width;

    InitWindow();
}
WsiWindow::~WsiWindow(){
    _WindowObj = nullptr;
}

bool WsiWindow::InitWindow(){

    if (glfwInit() != GLFW_TRUE) {
        FATAL("Create glfw window faild.");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _GLFWWindow = glfwCreateWindow(_Width, _Height, "RenderEngine", nullptr, nullptr);
    CHECK(_GLFWWindow);
    // Mouse Callback
     glfwSetScrollCallback(_GLFWWindow, scroll_callback);

    return true;
}

GLFWwindow* WsiWindow::GetWindow(){
   return _GLFWWindow;
}

WsiWindow* WsiWindow::GetInstance(){
    if(!_WindowObj){
        _WindowObj = new WsiWindow(1080, 720);
        CHECK(_WindowObj);
    }

    return _WindowObj;
}

void WsiWindow::DestoryWindow(){
    
    if(!_GLFWWindow) return;
    glfwDestroyWindow(_GLFWWindow);
    glfwTerminate();
}

void WsiWindow::GetWindowSize(int& w, int& h){
    w = _Width;
    h = _Height;
}
