#!/bin/bash

if [ -d "Build" ]; then
  mkdir Build
fi

cmake_cmd="cmake -DCMAKE_BUILD_TYPE=Debug ${PWD}"
build_cmd="make -j4 ${PWD}/Build"

$cmake_cmd && $build_cmd

