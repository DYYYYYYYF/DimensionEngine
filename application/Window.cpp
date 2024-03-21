#include"Window.hpp"

float scale_callback = 0.5;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (scale_callback + yoffset * 0.02 >= 0.1)
        scale_callback += static_cast<float>(yoffset * 0.02);
}

float WsiWindow::_Aspect = 0.0f;
int WsiWindow::_Height = 0;
int WsiWindow::_Width = 0;
WsiWindow* WsiWindow::_WindowObj = nullptr;
GLFWwindow* WsiWindow::_GLFWWindow = nullptr;

WsiWindow::WsiWindow() {

    if (_Width == 0) {
        _Width = 1600;
    }

    _WindowObj = nullptr;
    _Aspect = 16.0f / 9.0f;
    _Height = static_cast<int>(_Width / _Aspect);

    InitWindow();
}

WsiWindow::WsiWindow(int width) {
    _WindowObj = nullptr;
    _Aspect = 16.0f / 9.0f;
    _Height = static_cast<int>(width / _Aspect);
    _Width = width;

    InitWindow();
}

WsiWindow::WsiWindow(int width, int height){
    _WindowObj = nullptr;
    _Aspect = 16.0f / 9.0f;
    _Height = height;
    _Width = width;

    InitWindow();
}
WsiWindow::~WsiWindow(){
    _WindowObj = nullptr;
}

bool WsiWindow::InitWindow(){

    if (glfwInit() != GLFW_TRUE) {
        CoreLog("Create glfw window faild.");
        return false;
    }

    if (_Width <= 0 || _Height <= 0) {
        CoreLog("Invalid with:%d or invalid height:%d !", _Width, _Height);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _GLFWWindow = glfwCreateWindow(_Width, _Height, "RenderEngine", nullptr, nullptr);
    ASSERT(_GLFWWindow);
    // Mouse Callback
     glfwSetScrollCallback(_GLFWWindow, scroll_callback);

     GLFWimage icon;
     glfwSetWindowIcon(_GLFWWindow, 1, &icon);

    return true;
}

GLFWwindow* WsiWindow::GetWindow(){
   return _GLFWWindow;
}

WsiWindow* WsiWindow::GetInstance(){
    if(!_WindowObj){
        _WindowObj = new WsiWindow();
    }
    ASSERT(_WindowObj);

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
