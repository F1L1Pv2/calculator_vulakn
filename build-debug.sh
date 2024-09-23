#!/bin//sh
set -xe

sh ./build-shaders.sh
clang -std=c++17 -g -D DEBUG -o calc.exe src/main.cpp src/Window.cpp src/3dmath.cpp -I "./src/" -I "C:/VulkanSDK/1.3.290.0/Include" -lvulkan-1 -lkernel32 -luser32 -lgdi32
sh ./build-refresh.sh