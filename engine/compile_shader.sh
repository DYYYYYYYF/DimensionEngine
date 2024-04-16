#!/bin/bash

shader_path="../shader"

echo "Shader dir: ${shader_path}"

cmd="python ./script/compile_shader.py"
$cmd
