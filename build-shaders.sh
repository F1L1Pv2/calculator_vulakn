#!/bin/sh

set -x

glslc -fshader-stage=vert ./src/assets/vertex.glsl -o ./src/assets/vertex.spv
glslc -fshader-stage=frag ./src/assets/fragment.glsl -o ./src/assets/fragment.spv
python ./gen_assets.py