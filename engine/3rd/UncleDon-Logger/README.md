# UncleDon-Logger
A Logger with C++  

CMakeFile  

add include_dirctions and lib to yourself project  
Environment: Linux / Unix / Windows
# Method  
UL_DEBUG()  
UL_INFO()  
UL_WARN()  
UL_ERROR()  
UL_FATAL()  
  
# Some Suggestions
before use UncleDon-Logger  

include ./include direction
link lib/${plant_lib}/Logger.*

set first logFilename: `Log::Logger::getInstance()->open("today") `
  
# CMake && Make  
Single build:
Windows：  
```shell
  CMake .. - G "Visual Studio 16 2019"  
  Generate .sln file under build  
```
    
Linux/MacOS:  
```shell
  CMake ..  
  make  
```

Add to project:
```cmake
add_subdirectory(UncleDon-Logger)
```