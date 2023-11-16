import os
import platform

if platform.system().lower() == "windows":
    vulkanPath = "D:\Library\Vulkan"
    if os.system("glslc -v") != 0:
        if os.system(vulkanPath + "\Bin\glslc.exe --version") != 0:
            os.system("cls")
            print("Please set vulkan path!")
            quit()

filePath = os.path.abspath(".") + "/shader/"
print("Shader dir: " + filePath)

failedShaders = []

for root, dirs, files in os.walk(filePath):
    for file in files:
        filename = os.path.splitext(file)
        if filename[1] != ".spv":
            if filename[1] == ".vert":
                ## Record compile command
                cmd = ""
                if platform.system().lower() == 'windows':
                    cmd = vulkanPath + "\Bin\glslc.exe -c " + filePath + file + " -o " + filePath + filename[0] + "_vert.spv"
                else:
                    cmd = "glslc -c " + filePath + file + " -o " + filePath + filename[0] + "_vert.spv"

                print("Compiling " + filename[0] + ".vert")
                ## Execute compile command
                if os.system(cmd) != 0:
                    failedShaders.append(file)
            elif filename[1] == ".frag":
                ## Record compile command
                if platform.system().lower() == 'windows':
                    cmd = vulkanPath + "\Bin\glslc.exe -c " + filePath + file + " -o " + filePath + filename[0] + "_frag.spv"
                else:
                    cmd = "glslc -c " + filePath + file + " -o " + filePath + filename[0] + "_frag.spv"

                print("Compiling " + filename[0] + ".frag")
                ## Execute compile command
                if os.system(cmd) != 0:
                    failedShaders.append(file)

                        

if len(failedShaders) == 0:
    print("\nAll shaders compiled successful...")
else:
    print("\nComile failed shaders:")
    for failShader in failedShaders:
        print(failShader)
