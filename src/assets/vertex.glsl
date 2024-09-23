#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

struct InstanceData{
    mat4 model;
    vec2 tiling;
    vec2 offset;
};

layout(binding = 2) readonly buffer InstanceBufferObject {
    InstanceData data[1024];
} ibo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 fragUv;

void main() {
    gl_Position = ubo.proj * ubo.view * ibo.data[gl_InstanceIndex].model * vec4(inPosition, 1.0);
    fragUv = inUv * ibo.data[gl_InstanceIndex].tiling + ibo.data[gl_InstanceIndex].offset;
}