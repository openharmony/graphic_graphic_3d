/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_ADAPTER_INTF_CAMERA_UTILS_H
#define SCENE_ADAPTER_INTF_CAMERA_UTILS_H
#include <cstddef>
namespace OHOS::Render3D {

class Vector3f {
public:
    static constexpr size_t V3F_SIZE = 3;
    union {
        struct {
            float x, y, z;
        };
        float data[3];
    };

    constexpr Vector3f() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr Vector3f(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
    explicit constexpr Vector3f(float s) noexcept : x(s), y(s), z(s) {}

    // return the last element (z) if index is out of bounds
    constexpr float& operator[](size_t index) noexcept {
        return data[index >= V3F_SIZE ? V3F_SIZE - 1 : index];
    }
    constexpr const float& operator[](size_t index) const noexcept {
        return data[index >= V3F_SIZE ? V3F_SIZE - 1 : index];
    }
};

static_assert(sizeof(Vector3f) == 3 * sizeof(float), "Vector3f size mismatch");
static_assert(offsetof(Vector3f, x) == 0, "x offset error");
static_assert(offsetof(Vector3f, y) == sizeof(float), "y offset error");
static_assert(offsetof(Vector3f, z) == 2 * sizeof(float), "z offset error");

class Vector4f {
public:
    static constexpr size_t V4F_SIZE = 4;
    union {
        struct {
            float x, y, z, w;
        };
        float data[4];
    };

    constexpr Vector4f() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    constexpr Vector4f(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}
    explicit constexpr Vector4f(float s) noexcept : x(s), y(s), z(s), w(s) {}

    constexpr float& operator[](size_t index) noexcept {
        return data[index >= V4F_SIZE ? V4F_SIZE - 1 : index];
    }

    constexpr const float& operator[](size_t index) const noexcept {
        return data[index >= V4F_SIZE ? V4F_SIZE - 1 : index];
    }
};

static_assert(sizeof(Vector4f) == 4 * sizeof(float), "Vector4f size mismatch");
static_assert(offsetof(Vector4f, x) == 0, "x offset error");
static_assert(offsetof(Vector4f, y) == sizeof(float), "y offset error");
static_assert(offsetof(Vector4f, z) == 2 * sizeof(float), "z offset error");
static_assert(offsetof(Vector4f, w) == 3 * sizeof(float), "w offset error");



struct CameraIntrinsics {
    CameraIntrinsics(float fov = 1, float near = 0.1, float far = 10) :
                              fov_(fov), near_(near), far_(far) {}
    float fov_ = 1;
    float near_ = 0.1;
    float far_ = 10;
};

struct CameraConfigs {
    // camera pose
    Vector3f position_ = {0, 0, 0};
    Vector4f rotation_ = {0, 0, 0, 1};

    CameraIntrinsics intrinsics_;
    Vector4f clearColor_ = {0, 0, 0, 1}; // RGBA = black opaque

    std::string Dump()
    {
        std::string ret = "OffscreenCamera:[";
        ret += " position_: " + std::to_string(position_.x) + '\t' + std::to_string(position_.y) + '\t' +
            std::to_string(position_.z) + '\t';
        ret += " rotation_: " + std::to_string(rotation_.x) + '\t' + std::to_string(rotation_.y) + '\t' +
                std::to_string(rotation_.z) + '\t' + std::to_string(rotation_.w) + '\t';
        ret += "clearColor: " + std::to_string(clearColor_.x) + '\t' + std::to_string(clearColor_.y) + '\t' +
            std::to_string(clearColor_.z) + '\t' + std::to_string(clearColor_.w) + '\t';
        ret += "fov: " + std::to_string(intrinsics_.fov_) + "near: " + std::to_string(intrinsics_.near_) +
            "far: " + std::to_string(intrinsics_.far_);
        ret += "]";
        return ret;
    }
};

} // namespace OHOS::Render3D
#endif // SCENE_ADAPTER_INTF_CAMERA_UTILS_H
