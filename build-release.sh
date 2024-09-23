#!/bin/sh
set -xe

sh ./build-shaders.sh
sh ./build-release-assets.sh
clang -std=c++17 -ffunction-sections -fdata-sections -Oz -target x86_64-pc-windows-msvc -o calc.exe src/main.cpp src/Window.cpp src/3dmath.cpp -I "./src/" -I "C:/VulkanSDK/1.3.290.0/Include" -lvulkan-1 -lkernel32 -luser32 -lgdi32 -Wl,/subsystem:windows
llvm-strip calc.exe
upx --best --lzma calc.exe