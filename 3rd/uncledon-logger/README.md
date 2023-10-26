# UncleDon-Logger
A Logger with C++  

CMakeFile  

add include_dirctions and lib to yourself project  
Environment: Linux / Unix / Windows
# Method  
DEBUG()  
INFO()  
WARN()  
ERROR()  
FATAL()  
  
# Some Suggestions
before use UncleDon-Logger  

include ./include direction
link lib/${plant_lib}/Logger.*

set first logFilename just like main.cpp
  
# CMake && Make  
Windows：  
  CMake .. - G "Visual Studio 16 2019"  
  Generate .sln file under build  
    
Linux/MacOS:  
  CMake ..  
  make  
