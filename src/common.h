#ifndef COMMON_COMMON
#define COMMON_COMMON

#include "3dmath.h"

#ifdef DEBUG
#define DEBUG_MSG(str, ...) printf((const char* const)str, __VA_ARGS__)
#else
#define DEBUG_MSG(str, ...)
#endif


struct alignas(16) UniformBufferObject {
    mat4 view = mat4::identity();
    mat4 proj = mat4::identity();
};

struct alignas(16) InstanceData{
    mat4 model = mat4::identity();
    vec2 tiling = {1.0f,1.0f};
    vec2 offset = {0.0f,0.0f};
};

#define MAX_INSTANCES 1024

struct alignas(16) InstanceBufferObject {
    InstanceData data[MAX_INSTANCES];
};


struct Texture;
struct MeshView;

#endif