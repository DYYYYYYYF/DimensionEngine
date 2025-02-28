## Dimension Engine
3D-Rendering of Vulkan 

git clone --recursive git@github.com:DYYYYYYYF/DimensionEngine.git  

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

## Plugins

#### Audio: 

``` cmake
set(ENABLE_PLUGINS_AUDIO ON)
```

OpenAL and sniffle lib to load audio resource (.wma) and play.

## Console

| Key  | Description                                        |
| ---- | -------------------------------------------------- |
| `    | Toggle console command line status. (Write / None) |

#### Command Mode

| Key  | Description             |
| ---- | ----------------------- |
| Up   | Move up console text.   |
| Down | Move down console text. |

#### Commands

| Name | Description       |
| ---- | ----------------- |
| quit | Quit application. |
| exit | Quit application. |

## Short cut

PBR  
![](Assets/Shortcuts/PBR.png)



## 3rd-Libraries

Vulkan: https://www.vulkan.org

UncleDon-Logger: https://github.com/DYYYYYYYF/UncleDon-Logger

stb: https://github.com/nothings/stb/tree/master  

gltf：https://github.com/syoyo/tinygltf  

audio：  
https://github.com/kcat/openal-soft  
https://github.com/libsndfile/libsndfile 

JSON：https://github.com/Tencent/rapidjson

