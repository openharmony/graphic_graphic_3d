#ifndef SHADERKIND_H
#define SHADERKIND_H

enum class ShaderEnv {
    version_vulkan_1_0,
    version_vulkan_1_1,
    version_vulkan_1_2,
    version_vulkan_1_3,
};

enum class ShaderKind {
    VERTEX,
    FRAGMENT,
    GEOMETRY,
    COMPUTE,
};

#endif