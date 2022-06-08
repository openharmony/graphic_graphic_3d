/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef GLES_SURFACE_INFORMATION_H
#define GLES_SURFACE_INFORMATION_H

#include <cstdint>

#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
namespace GlesImplementation {
struct SurfaceInfo {
    uint32_t configId { 0 };
    uint32_t width { 0 }, height { 0 };
    uint32_t red_size { 0 }, green_size { 0 }, blue_size { 0 }, alpha_size { 0 };
    uint32_t depth_size { 0 };
    uint32_t stencil_size { 0 };
    uint32_t samples { 0 };
    bool srgb { false };
};
} // namespace GlesImplementation
RENDER_END_NAMESPACE()

#endif // GLES_SURFACE_INFORMATION_H
