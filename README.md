# Dimension Engine
3D-Rendering of Vulkan 

CMake Files: (MacOS required Vulkan,GLM,GLFW)

create a floder named build

cd build then cmd: cmake [[Custom params]](### Cmake params) ..

cmd 'make'

after all, it will create a executable program named VulkanTestDemo

Windows: Visual Studio 2022

### Cmake params

-DCMAKE_BUILD_TYPE=[CMAKE_BUILD_TYPE]
-DLOG_LEVEL=[LOG_LEVEL]

**CMAKE_BUILD_TYPE:**

* <font color=#00a8ff>Release</font>
* <font color=#00a8ff>Debug</font>

**LOG_LEVEL:**

* <font color=#88888888>INFO </font>
* <font color=#6c3d2c>DEBUG </font>
* <font color=Yellow>WARN </font>
* <font color=Red>ERROR</font>
* <font color=#8b0000>FATAL </font>

## Warn: Under refactory 
By far, impl Triangle renderer.

Renderer: Instance Device Swapchain FrameBuffer RenderPass CommandBuffer

The foundational functions have finished.

If your plants form is Windows, you could compile shaders by compile_shader.bat. (Dont forget set current VulkanSDK path)

## 3rd-Libraries

Vulkan: https://www.vulkan.org

UncleDon-Logger: https://github.com/DYYYYYYYF/UncleDon-Logger

ThreadPool: https://github.com/DYYYYYYYF/ThreadPool

