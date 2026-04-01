
#ifndef SCENE_ADAPTER_INTF_CAMERA_UTILS_H
#define SCENE_ADAPTER_INTF_CAMERA_UTILS_H
#include <cstddef>
namespace OHOS::Render3D {

class Vector3f {
public:
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
        return data[index >= 3 ? 2 : index];
    }
    constexpr const float& operator[](size_t index) const noexcept {
        return data[index >= 3 ? 2 : index];
    }
};

static_assert(sizeof(Vector3f) == 3 * sizeof(float), "Vector3f size mismatch");
static_assert(offsetof(Vector3f, x) == 0, "x offset error");
static_assert(offsetof(Vector3f, y) == sizeof(float), "y offset error");
static_assert(offsetof(Vector3f, z) == 2 * sizeof(float), "z offset error");

class Vector4f {
public:
    union {
        struct {
            float x, y, z, w;
        };
        float data[4];
    };

    constexpr Vector4f() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    constexpr Vector4f(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}
    explicit constexpr Vector4f(float s) noexcept : x(s), y(s), z(s), w(s) {}

    // 非常量下标运算符，越界时返回最后一个元素（w）
    constexpr float& operator[](size_t index) noexcept {
        return data[index >= 4 ? 3 : index];
    }

    // 常量下标运算符，越界时返回最后一个元素（w）
    constexpr const float& operator[](size_t index) const noexcept {
        return data[index >= 4 ? 3 : index];
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
        ret += " position_: " + std::to_string(position_[0]) + '\t' + std::to_string(position_[1]) + '\t' +
            std::to_string(position_[2]) + '\t';
        ret += " rotation_: " + std::to_string(rotation_[0]) + '\t' + std::to_string(rotation_[1]) + '\t' +
                std::to_string(rotation_[2]) + '\t' + std::to_string(rotation_[3]) + '\t';
        ret += "clearColor: " + std::to_string(clearColor_[0]) + '\t' + std::to_string(clearColor_[1]) + '\t' +
            std::to_string(clearColor_[2]) + '\t' + std::to_string(clearColor_[3]) + '\t';
        ret += "fov: " + std::to_string(intrinsics_.fov_) + "near: " + std::to_string(intrinsics_.near_) +
            "far: " + std::to_string(intrinsics_.far_);
        ret += "]";
        return ret;
    }
};

} // namespace OHOS::Render3D
#endif // SCENE_ADAPTER_INTF_CAMERA_UTILS_H
