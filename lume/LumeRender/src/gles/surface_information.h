/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
