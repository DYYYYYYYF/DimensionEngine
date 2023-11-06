import os

filePath = os.path.abspath(".") + "/shader/"
print("Shader dir: " + filePath)

failedShaders = []

for root, dirs, files in os.walk(filePath):
    for file in files:
        filename = os.path.splitext(file)
        if filename[1] != ".spv":
            if filename[1] == ".vert":
                ## Record compile command
                cmd = "D:\Library\Vulkan\Bin\glslc.exe -c " + filePath + file + " -o " + filePath + filename[0] + "_vert.spv"

                print("Compiling " + filename[0] + ".vert")
                ## Execute compile command
                if os.system(cmd) != 0:
                    failedShaders.append(file)
            elif filename[1] == ".frag":
                ## Record compile command
                cmd = "D:\Library\Vulkan\Bin\glslc.exe -c " + filePath + file + " -o " + filePath + filename[0] + "_frag.spv"

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