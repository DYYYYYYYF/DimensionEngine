import os
import platform

# Common values
vulkanPath       = "D:\C_Library\VulkanSDK"
compile_command  = "glslc -c "
file_path        = os.path.abspath(".") + "/shader/"

# compile failed files
failed_shaders = []

def CheckGlslc():
    # global value will be changed, so we need define the value inside func
    global compile_command

    compile_check_command     = compile_command + "--version"
    if os.system(compile_check_command) != 0:
        compile_check_command = file_path + "glslc.exe -c "
        
        if os.system(compile_check_command) != 0:
            # Not set global command
            os.system("cls")
            compile_command = vulkanPath + "\Bin\glslc.exe -c "
            compile_check_command = compile_command + "--version "
            
            if os.system(compile_check_command) != 0:
                # Not set right vulkan path
                os.system("cls")
                print("Please set vulkan path!")
                quit()


def Compile(sub_file_path, file, filename):
    shader_type = filename[1].split('.')[1]
    if filename[1] != ".hlsl":
        ## Record compile command
        cmd = compile_command + sub_file_path + "/" + file + " -o " + sub_file_path + "/" + filename[0] + "_" + shader_type + ".spv"
        print("Compiling " + sub_file_path + "/" + filename[0] + filename[1])

        ## Execute compile command
        if os.system(cmd) != 0:
            failed_shaders.append(file)

    else:
        sub_filename = os.path.splitext(filename[0])
        shader_type = sub_filename[1].split('.')[1]

        ## Record compile command
        cmd = compile_command + " -c -fshader-stage=" + shader_type + " -fentry-point=main " + sub_file_path \
        + "/" + file + " -o " + sub_file_path + "/" + filename[0] + ".spv"
        print("Compiling " + sub_file_path + "/" + filename[0] + filename[1])

        ## Execute compile command
        if os.system(cmd) != 0:
            failed_shaders.append(file)


def SearchShaderFiles(sub_file_path):
    for root, dirs, files in os.walk(sub_file_path):
        for dir in dirs:
            SearchShaderFiles(dir)
        for file in files:
            filename = os.path.splitext(file)
            if filename[1] != ".spv" and filename[1] != ".exe" and filename[1] != '':
                Compile(root, file, filename)

def CompileShaders():
    global failed_shaders
    
    # Windows platform
    # Check glslc command
    if platform.system().lower() == "windows":
        CheckGlslc()

    print("Shader direction: " + file_path + "\n")
    SearchShaderFiles(file_path)

    LogCompileResult()
    

def LogCompileResult():
    if len(failed_shaders) == 0:
        print("\nAll shaders compiled successful...")
    else:
        print("\nComile failed shaders:")
        for fail_hader in failed_shaders:
            print(fail_hader)


if __name__ == '__main__':
    CompileShaders()
