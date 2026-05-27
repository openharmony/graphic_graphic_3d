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

#ifndef API_3D_GLTF_GLTF_DATA_H
#define API_3D_GLTF_GLTF_DATA_H

#include <cstdint>

#include <base/containers/atomics.h>
#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
namespace GLTF2 {
constexpr const uint32_t GLTF_INVALID_INDEX = 0xFFFFFFFF;

struct Skin;
struct AttributeBase;
struct Node;
struct KHRLight;

enum class BufferTarget : int {
    NOT_DEFINED = 0,
    ARRAY_BUFFER = 34962,
    ELEMENT_ARRAY_BUFFER = 34963,
};

enum class AttributeType : int {
    NORMAL = 0,
    POSITION = 1,
    TANGENT = 2,
    TEXCOORD = 3,
    COLOR = 4,
    JOINTS = 5,
    WEIGHTS = 6,
    INVALID = 0xff
};

/** Primitive rendering mode. Values correspond to WebGL enums. */
enum class RenderMode : int {
    BEGIN = 0,
    POINTS = 0,
    LINES = 1,
    LINE_LOOP = 2,
    LINE_STRIP = 3,
    TRIANGLES = 4,
    TRIANGLE_STRIP = 5,
    TRIANGLE_FAN = 6,
    COUNT = 7,
    INVALID = 0xff
};

constexpr const RenderMode DEFAULT_RENDER_MODE{RenderMode::TRIANGLES};

/** The datatype of components in an attribute. Values correspond to WebGL enums. */
enum class ComponentType : int {
    INVALID,
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    INT = 5124,
    UNSIGNED_INT = 5125,
    FLOAT = 5126,
};

/** Specifies if the attribute is a scalar, vector, or matrix. */
enum class DataType : int { INVALID, SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4 };

enum class AlphaMode : int {
    /** The alpha value is ignored and the rendered output is fully opaque. */
    OPAQUE_ALPHA,
    /** The rendered output is either fully opaque or fully transparent depending on the alpha value
     *  and the specified alpha cutoff value. */
    MASK_ALPHA,
    /** The alpha value is used to composite the source and destination areas using the normal
     *  painting operation (Porter and Duff over operator). */
    BLEND_ALPHA,
};

enum class MimeType : int { INVALID, JPEG, PNG, KTX, DDS, KTX2 };

enum class CameraType : int { INVALID, PERSPECTIVE, ORTHOGRAPHIC };

enum class LightType : int { INVALID, DIRECTIONAL, POINT, SPOT, AMBIENT };

enum class FilterMode : int {
    NEAREST = 9728,
    LINEAR = 9729,
    NEAREST_MIPMAP_NEAREST = 9984,
    LINEAR_MIPMAP_NEAREST = 9985,
    NEAREST_MIPMAP_LINEAR = 9986,
    LINEAR_MIPMAP_LINEAR = 9987
};

enum class WrapMode : int { CLAMP_TO_EDGE = 33071, MIRRORED_REPEAT = 33648, REPEAT = 10497 };

enum class AnimationInterpolation : int { INVALID, STEP, LINEAR, SPLINE };

enum class AnimationPath : int {
    INVALID,
    TRANSLATION,
    ROTATION,
    SCALE,
    WEIGHTS,
    VISIBLE,
    OPACITY,
};

enum class CompressionMode : int {
    ATTRIBUTES = 0,
    TRIANGLES = 1,
    INDICES = 2,
    INVALID = 0xff,
};

enum class CompressionFilter : int {
    NONE = 0,
    OCTAHEDRAL = 1,
    QUATERNION = 2,
    EXPONENTIAL = 3,
    INVALID = 0xff,
};

struct Buffer {
    /** Required. Buffer byte length. */
    size_t byteLength = 0;

    /** URI to the buffer data. Either empty (GLB buffer), a file path, or a data-URI. */
    BASE_NS::string uri;

    /** Raw data for this buffer. Populated after LoadBuffers(). */
    BASE_NS::vector<uint8_t> data;
};

struct BufferView {
    /** The buffer this view references. Buffers must be loaded first. */
    Buffer* buffer{nullptr};

    /** Required. Byte length of this view. */
    size_t byteLength = 0;

    /** Byte offset into the buffer. Default: 0. */
    size_t byteOffset = 0;

    /** Stride in bytes between vertex attributes. 0 means tightly packed.
     *  When two or more accessors share this view, stride must be defined. */
    size_t byteStride = 0;

    BufferTarget target = BufferTarget::NOT_DEFINED;

    /** Pointer to buffer data for this view. Valid after LoadBuffers(). */
    const uint8_t* data{nullptr};

    /** Meshopt compression data (EXT_meshopt_compression). */
    struct MeshOptCompression {
        Buffer* buffer{nullptr};
        size_t byteLength = 0;
        size_t byteOffset = 0;
        size_t byteStride = 0;
        size_t count = 0;
        CompressionMode mode = CompressionMode::INVALID;
        CompressionFilter filter = CompressionFilter::NONE;
        /** Decompressed data. Populated lazily on first access. */
        BASE_NS::vector<uint8_t> data;
        BASE_NS::SpinLock dataLock;
    } meshoptCompression;

    BufferView() = default;
    BufferView(const BufferView& other)
        : buffer(other.buffer),
          byteLength(other.byteLength),
          byteOffset(other.byteOffset),
          byteStride(other.byteStride),
          target(other.target),
          data(other.data)
    {
        meshoptCompression.buffer = other.meshoptCompression.buffer;
        meshoptCompression.byteLength = other.meshoptCompression.byteLength;
        meshoptCompression.byteOffset = other.meshoptCompression.byteOffset;
        meshoptCompression.byteStride = other.meshoptCompression.byteStride;
        meshoptCompression.count = other.meshoptCompression.count;
        meshoptCompression.mode = other.meshoptCompression.mode;
        meshoptCompression.filter = other.meshoptCompression.filter;
        meshoptCompression.data = other.meshoptCompression.data;
    }
};

struct SparseIndices {
    BufferView* bufferView{nullptr};
    uint32_t byteOffset = 0;
    ComponentType componentType = ComponentType::UNSIGNED_INT;
};

struct SparseValues {
    BufferView* bufferView{nullptr};
    uint32_t byteOffset = 0;
};

/** Sparse storage of accessor attributes that deviate from their initialization value. */
struct Sparse {
    /** Number of sparse attributes. */
    uint32_t count = 0;
    /** Index array pointing to deviating attributes. Indices must strictly increase. */
    SparseIndices indices;
    /** Array of substitute attribute values. */
    SparseValues values;
};

struct Accessor {
    /** The buffer view containing the data. When null, accessor must be initialized with zeros. */
    BufferView* bufferView{nullptr};

    /** Required. Component data type (BYTE, FLOAT, etc.). */
    ComponentType componentType = ComponentType::UNSIGNED_INT;

    /** Required. Number of elements (not bytes or components). */
    uint32_t count = 0;

    /** Required. Element type (SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4). */
    DataType type = DataType::INVALID;

    /** Byte offset into the buffer view. Must be a multiple of component size. */
    uint32_t byteOffset = 0;

    /** Whether integer values should be normalized to [0,1] or [-1,1]. */
    bool normalized = false;

    /** Per-component maximum values. Length determined by type (1..16). */
    BASE_NS::vector<float> max;

    /** Per-component minimum values. Length determined by type (1..16). */
    BASE_NS::vector<float> min;

    /** Sparse storage for attributes deviating from initialization value. */
    Sparse sparse;
};

struct AttributeBase {
    AttributeType type{AttributeType::INVALID};
    uint32_t index{0};  // e.g. texcoord set 0, 1, 2...
};

struct Attribute {
    AttributeBase attribute;
    Accessor* accessor{nullptr};
};

struct Image {
    /** URI to the image. Relative paths are relative to the .gltf file.
     *  Can also be a data-URI. */
    BASE_NS::string uri;

    /** The buffer view containing the image data (alternative to uri). */
    BufferView* bufferView{nullptr};

    /** The image's MIME type. Required when bufferView is used. */
    MimeType type{MimeType::INVALID};
};

struct Sampler {
    FilterMode magFilter = FilterMode::LINEAR;
    FilterMode minFilter = FilterMode::LINEAR;
    WrapMode wrapS = WrapMode::REPEAT;
    WrapMode wrapT = WrapMode::REPEAT;
};

struct Texture {
    /** The sampler. When null, a sampler with repeat wrapping and auto filtering should be used. */
    Sampler* sampler{nullptr};
    /** The image used by this texture. */
    Image* image{nullptr};
};

struct TextureInfo {
    Texture* texture{nullptr};
    /** @internal Parse-time texture index. */
    uint32_t index = GLTF_INVALID_INDEX;
    /** The TEXCOORD attribute set index for texture coordinate mapping. Default: 0. */
    uint32_t texCoordIndex = 0;

    /** KHR_texture_transform extension data. */
    struct TextureTransform {
        BASE_NS::Math::Vec2 offset{0.0f, 0.0f};
        BASE_NS::Math::Vec2 scale{1.f, 1.f};
        float rotation = 0.0f;
        uint32_t texCoordIndex = GLTF_INVALID_INDEX;
    } transform;
};

struct MetallicRoughness {
    /** RGBA base color factor. Multiplied with base color texture if present. Default: [1,1,1,1]. */
    BASE_NS::Math::Vec4 baseColorFactor{1.f, 1.f, 1.f, 1.f};
    /** Base color texture (sRGB). */
    TextureInfo baseColorTexture;
    /** Metalness factor. 1.0 = metal, 0.0 = dielectric. Default: 1.0. */
    float metallicFactor{1.f};
    /** Roughness factor. 1.0 = completely rough, 0.0 = completely smooth. Default: 1.0. */
    float roughnessFactor{1.f};
    /** Metallic-roughness texture. B channel = metallic, G channel = roughness. */
    TextureInfo metallicRoughnessTexture;
};

struct NormalTexture {
    TextureInfo textureInfo;
    /** Normal map scale factor. Default: 1.0. */
    float scale = 1.0f;
};

struct OcclusionTexture {
    TextureInfo textureInfo;
    /** Occlusion strength. 0.0 = no occlusion, 1.0 = full occlusion. Default: 1.0. */
    float strength = 1.0f;
};

struct Material {
    enum class Type { MetallicRoughness, SpecularGlossiness, Unlit };

    Type type{Type::MetallicRoughness};
    BASE_NS::string name;
    MetallicRoughness metallicRoughness;
    NormalTexture normalTexture;
    OcclusionTexture occlusionTexture;
    TextureInfo emissiveTexture;
    BASE_NS::Math::Vec4 emissiveFactor = BASE_NS::Math::Vec4(0.f, 0.f, 0.f, 1.f);
    AlphaMode alphaMode = AlphaMode::OPAQUE_ALPHA;
    float alphaCutoff = 0.5f;
    /** When true, back-face culling is disabled and double-sided lighting is enabled. */
    bool doubleSided = false;

    /** KHR_materials_clearcoat */
    struct Clearcoat {
        float factor = 0.0f;
        TextureInfo texture;
        float roughness = 0.0f;
        TextureInfo roughnessTexture;
        NormalTexture normalTexture;
    } clearcoat;

    /** KHR_materials_ior */
    struct Ior {
        float ior = 1.5f;
    } ior;

    /** KHR_materials_pbrSpecularGlossiness */
    struct SpecularGlossiness {
        BASE_NS::Math::Vec4 diffuseFactor{1.f, 1.f, 1.f, 1.f};
        TextureInfo diffuseTexture;
        BASE_NS::Math::Vec3 specularFactor{1.f, 1.f, 1.f};
        float glossinessFactor = 1.0f;
        TextureInfo specularGlossinessTexture;
    } specularGlossiness;

    /** KHR_materials_sheen */
    struct Sheen {
        BASE_NS::Math::Vec3 factor;
        TextureInfo texture;
        float roughness = 0.0f;
        TextureInfo roughnessTexture;
    } sheen;

    /** KHR_materials_specular */
    struct Specular {
        float factor = 1.f;
        TextureInfo texture;
        BASE_NS::Math::Vec3 color{1.f, 1.f, 1.f};
        TextureInfo colorTexture;
    } specular;

    /** KHR_materials_transmission */
    struct Transmission {
        float factor = 0.0f;
        TextureInfo texture;
    } transmission;
};

struct MorphTarget {
    BASE_NS::string name;
    BASE_NS::vector<Attribute> target;
    bool iGfxCompressed = false;
};

struct MeshPrimitive {
    /** Vertex attributes (POSITION, NORMAL, TANGENT, TEXCOORD, etc.). */
    BASE_NS::vector<Attribute> attributes;

    /** Index accessor. When null, primitives should be rendered without indices. */
    Accessor* indices{nullptr};

    /** Material to apply when rendering. */
    Material* material{nullptr};

    /** @internal Parse-time material index. */
    uint32_t materialIndex = GLTF_INVALID_INDEX;

    /** Primitive topology. Default: TRIANGLES. */
    RenderMode mode = DEFAULT_RENDER_MODE;

    /** Morph targets (POSITION, NORMAL, TANGENT deltas). */
    BASE_NS::vector<MorphTarget> targets;
};

struct Mesh {
    BASE_NS::string name;
    BASE_NS::vector<MeshPrimitive> primitives;
    /** Morph target weights. */
    BASE_NS::vector<float> weights;
};

struct Camera {
    BASE_NS::string name;
    CameraType type{CameraType::INVALID};

    union Attributes {
        struct Perspective {
            float aspect;
            float yfov;  // required
            float zfar;
            float znear;  // required
        } perspective;

        struct Ortho {
            float xmag;   // required, non-zero
            float ymag;   // required, non-zero
            float zfar;   // required
            float znear;  // required
        } ortho;
    } attributes{};
};

struct Skin {
    BASE_NS::string name;
    /** Accessor containing floating-point 4x4 inverse-bind matrices. */
    Accessor* inverseBindMatrices{nullptr};
    /** Skeleton root node. When null, joints resolve to scene root. */
    Node* skeleton{nullptr};
    /** Joint nodes used by this skin. */
    BASE_NS::vector<Node*> joints;
};

struct Node {
    BASE_NS::string name;
    Mesh* mesh{nullptr};
    Camera* camera{nullptr};
    KHRLight* light{nullptr};
    BASE_NS::string modelIdRSDZ;

    Node* parent{nullptr};
    bool isJoint = false;

    BASE_NS::vector<Node*> children;
    /** @internal Parse-time child indices (resolved to children pointers after parsing). */
    BASE_NS::vector<size_t> tmpChildren;

    Skin* skin{nullptr};
    /** @internal Parse-time skin index. */
    uint32_t tmpSkin{~0u};

    bool usesTRS = true;
    BASE_NS::Math::Vec3 translation{0.f, 0.f, 0.f};
    BASE_NS::Math::Quat rotation{0.f, 0.f, 0.f, 1.f};
    BASE_NS::Math::Vec3 scale{1.f, 1.f, 1.f};
    BASE_NS::Math::Mat4X4 matrix;

    /** Morph target weights. */
    BASE_NS::vector<float> weights;
};

struct Scene {
    BASE_NS::string name;
    /** Root nodes of the scene. */
    BASE_NS::vector<Node*> nodes;
    /** Ambient light (KHR_lights_punctual). */
    KHRLight* light{nullptr};
    /** Image-based light index (EXT_lights_image_based). */
    size_t imageBasedLightIndex = GLTF_INVALID_INDEX;
};

struct AnimationSampler {
    /** Timestamps accessor (scalar float values in seconds). */
    Accessor* input{nullptr};
    /** Keyframe values accessor. */
    Accessor* output{nullptr};
    AnimationInterpolation interpolation{AnimationInterpolation::INVALID};
};

/** Animation channel target. */
struct AnimationChannel {
    Node* node{nullptr};
    AnimationPath path{AnimationPath::INVALID};
};

/** Animation channel (binds a sampler to a target). */
struct AnimationTrack {
    AnimationChannel channel;
    AnimationSampler* sampler{nullptr};
};

struct Animation {
    BASE_NS::string name;
    BASE_NS::vector<AnimationTrack> tracks;
    BASE_NS::vector<BASE_NS::unique_ptr<AnimationSampler>> samplers;
};

/** KHR_lights_punctual light definition. */
struct KHRLight {
    BASE_NS::string name;
    LightType type = LightType::DIRECTIONAL;
    BASE_NS::Math::Vec3 color = BASE_NS::Math::Vec3(1.f, 1.f, 1.f);
    /** Intensity in candela (point/spot) or lux (directional). Default: 1.0. */
    float intensity = 1.0f;

    struct {
        float range = .0f;
        struct {
            float innerAngle = 0.f;
            float outerAngle = 0.785398163397448f;  // PI / 4
        } spot;
    } positional;

    struct {
        bool shadowCaster = false;
        float nearClipDistance = 100.f;
        float farClipDistance = 10000.f;
    } shadow;
};

/** EXT_lights_image_based IBL environment. */
struct ImageBasedLight {
    using CubemapMipLevel = BASE_NS::vector<size_t>;
    using LightingCoeff = BASE_NS::vector<float>;

    BASE_NS::string name;
    /** IBL environment rotation. */
    BASE_NS::Math::Quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
    float intensity{1.0f};
    /** Spherical harmonic coefficients for irradiance (9x3 array). */
    BASE_NS::vector<LightingCoeff> irradianceCoefficients;
    /** Prefiltered cubemap mip levels (Nx6 array). */
    BASE_NS::vector<CubemapMipLevel> specularImages;
    /** First specular mip dimension in pixels. */
    uint32_t specularImageSize{0};
    size_t specularCubeImage{GLTF_INVALID_INDEX};
    size_t skymapImage{GLTF_INVALID_INDEX};
    float skymapImageLodLevel{0.0f};
};

/** Container for all parsed glTF data. */
struct GltfData {
    GltfData() = default;
    GltfData(const GltfData&) = delete;
    virtual ~GltfData() = default;

    /** Directory path the glTF was loaded from. */
    BASE_NS::string filepath;
    /** Filename of the loaded glTF (without path). */
    BASE_NS::string defaultResources;

    BASE_NS::unique_ptr<Material> defaultMaterial;
    BASE_NS::unique_ptr<Sampler> defaultSampler;
    Scene* defaultScene{nullptr};

    BASE_NS::vector<BASE_NS::unique_ptr<Buffer>> buffers;
    BASE_NS::vector<BASE_NS::unique_ptr<BufferView>> bufferViews;
    BASE_NS::vector<BASE_NS::unique_ptr<Accessor>> accessors;
    BASE_NS::vector<BASE_NS::unique_ptr<Mesh>> meshes;
    BASE_NS::vector<BASE_NS::unique_ptr<Camera>> cameras;
    BASE_NS::vector<BASE_NS::unique_ptr<Image>> images;
    BASE_NS::vector<BASE_NS::unique_ptr<Sampler>> samplers;
    BASE_NS::vector<BASE_NS::unique_ptr<Texture>> textures;
    BASE_NS::vector<BASE_NS::unique_ptr<Material>> materials;
    BASE_NS::vector<BASE_NS::unique_ptr<Node>> nodes;
    BASE_NS::vector<BASE_NS::unique_ptr<Scene>> scenes;
    BASE_NS::vector<BASE_NS::unique_ptr<Animation>> animations;
    BASE_NS::vector<BASE_NS::unique_ptr<Skin>> skins;
    BASE_NS::vector<BASE_NS::unique_ptr<KHRLight>> lights;

    struct Thumbnail {
        BASE_NS::string uri;
        BASE_NS::string extension;
        BASE_NS::vector<uint8_t> data;
    };
    BASE_NS::vector<Thumbnail> thumbnails;

    BASE_NS::vector<BASE_NS::unique_ptr<ImageBasedLight>> imageBasedLights;
    /** True if KHR_mesh_quantization extension is required. */
    bool quantization{false};
    /** True if EXT_meshopt_compression extension is required. */
    bool meshCompression{false};
};

}  // namespace GLTF2
CORE3D_END_NAMESPACE()

#endif  // API_3D_GLTF_GLTF_DATA_H
