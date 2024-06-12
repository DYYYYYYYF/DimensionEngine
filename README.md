o# Dimension Engine
3D-Rendering of Vulkan 

CMake Files: (MacOS required Vulkan)

create a floder named build

cd build then cmd: cmake [[Custom params]](### Cmake params) ..

cmd 'make'

after all, it will create a executable program named 'TestEditor'

Windows: Visual Studio 2022

If your plants form is Windows, you could compile shaders by compile_shader_glsl.bat. (Dont forget set current VulkanSDK path)

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

## Short cut

![](Assets/Shortcuts/DimensionEngine.png)



## 3rd-Libraries

Vulkan: https://www.vulkan.org

UncleDon-Logger: https://github.com/DYYYYYYYF/UncleDon-Logger

stb: https://github.com/nothings/stb/tree/ae721c50eaf761660b4f90cc590453cdb0c2acd0

