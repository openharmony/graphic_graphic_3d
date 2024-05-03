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

#ifndef CORE__GLTF__GLTF2_DATA_STRUCTURES_H
#define CORE__GLTF__GLTF2_DATA_STRUCTURES_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/namespace.h>

#define GLTF2_EXTENSION_IGFX_COMPRESSED
#define GLTF2_EXTENSION_KHR_LIGHTS
#define GLTF2_EXTENSION_KHR_LIGHTS_PBR
#define GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT
#define GLTF2_EXTENSION_KHR_MATERIALS_EMISSIVE_STRENGTH
#define GLTF2_EXTENSION_KHR_MATERIALS_IOR
#define GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS
#define GLTF2_EXTENSION_KHR_MATERIALS_SHEEN
#define GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR
#define GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION
#define GLTF2_EXTENSION_KHR_MATERIALS_UNLIT
#define GLTF2_EXTENSION_KHR_MESH_QUANTIZATION
#define GLTF2_EXTENSION_KHR_TEXTURE_BASISU
#define GLTF2_EXTENSION_KHR_TEXTURE_TRANSFORM
#define GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED
#define GLTF2_EXTRAS_CLEAR_COAT_MATERIAL
#define GLTF2_EXTENSION_HW_XR_EXT
#define GLTF2_EXTRAS_RSDZ

#ifdef OPAQUE
// hrpm win32 gdi..
#undef OPAQUE
#endif

CORE3D_BEGIN_NAMESPACE()
namespace GLTF2 {
constexpr const uint32_t GLTF_INVALID_INDEX = 0xFFFFFFFF;
constexpr const uint32_t GLTF_MAGIC = 0x46546C67; // ASCII string "glTF"

struct Skin;
struct AttributeBase;
struct Node;

// extensions
#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
struct KHRLight;
#endif

enum class BufferTarget : int {
    NOT_DEFINED = 0,
    ARRAY_BUFFER = 34962,
    ELEMENT_ARRAY_BUFFER = 34963,
};

enum class ChunkType : int {
    JSON = 0x4E4F534A, //  JSON
    BIN = 0x004E4942,  //  BIN Binary buffer
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

enum class RenderMode : int {
    // WebGL enums
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

enum class ComponentType : int {
    INVALID,
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    INT = 5124, // not used in GLTF2
    UNSIGNED_INT = 5125,
    FLOAT = 5126,
};

enum class DataType : int { INVALID, SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4 };

enum class AlphaMode : int {
    // The alpha value is ignored and the rendered output is fully opaque.
    OPAQUE,
    // The rendered output is either fully opaque or fully transparent
    // depending on the alpha value and the specified alpha cutoff value.
    MASK,
    // The alpha value is used to composite the source and destination areas.
    // The rendered output is combined with the background using the normal
    // painting operation (i.e. the Porter and Duff over operator).
    BLEND,
};

enum class BlendMode : int {
    TRANSPARENT_ALPHA, // transparentAlpha
    TRANSPARENT_COLOR, // transparentColor
    ADD,               // add
    MODULATE,          // modulate
    REPLACE,           // replace
    NONE               // use alphaMode if blendMode not mentioned, (default)
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
    // RDSZ specific
    VISIBLE,
    OPACITY,
};

struct GLBHeader {
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t length = 0;
};

struct GLBChunk {
    uint32_t chunkLength = 0;
    uint32_t chunkType = 0;
};

struct Buffer {
    // [required field]
    size_t byteLength = 0;

    // either empty (indicating GLB buffer)
    // or path to file
    // or data:[<mediatype>][;base64],<data> as defined
    // in https://tools.ietf.org/html/rfc2397
    BASE_NS::string uri;

    // Data for this buffer.
    BASE_NS::vector<uint8_t> data;
};

struct BufferView {
    // [required field], with the index to the buffer.
    // Note: referenced buffers needs to be loaded first.
    Buffer* buffer { nullptr };

    // required, "minimum": 1
    size_t byteLength = 0;

    // "minimum": 0, "default": 0
    size_t byteOffset = 0;

    // "minimum": 4, "maximum": 252, "multipleOf": 4
    // The stride, in bytes, between vertex attributes.
    // When this is not defined (0), data is tightly packed.
    // When two or more accessors use the same bufferView, this field must be defined.
    size_t byteStride = 0;

    BufferTarget target = BufferTarget::NOT_DEFINED; // ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER

    // Data for this buffer view.
    const uint8_t* data { nullptr };
};

struct SparseIndices {
    // The bufferView with sparse indices.
    // Referenced bufferView can't have ARRAY_BUFFER or
    // ELEMENT_ARRAY_BUFFER target.
    BufferView* bufferView { nullptr };

    // The offset relative to the start of the bufferView in bytes.
    // Must be aligned.
    // "minimum": 0,
    // "default": 0
    uint32_t byteOffset = 0;

    // The indices data type.
    // Valid values correspond to WebGL enums:
    // `5121` means UNSIGNED_BYTE, `5123` means UNSIGNED_SHORT
    // `5125` means UNSIGNED_INT.
    ComponentType componentType = ComponentType::UNSIGNED_INT;
};

struct SparseValues {
    // The bufferView with sparse values.
    // Referenced bufferView can't have ARRAY_BUFFER or
    // ELEMENT_ARRAY_BUFFER target."
    BufferView* bufferView { nullptr };

    // The offset relative to the start of the bufferView in bytes.
    // Must be aligned.
    uint32_t byteOffset = 0;
};

struct Sparse {
    // The number of attributes encoded in this sparse accessor.
    uint32_t count = 0;

    // Index array of size `count` that points to those accessor attributes
    // that deviate from their initialization value. Indices must strictly increase.
    SparseIndices indices;

    // Array of size `count` times number of components,
    // storing the displaced accessor attributes pointed by `indices`.
    // Substituted values must have the same `componentType` and
    // number of components as the base accessor."
    SparseValues values;
};

struct Accessor {
    // The bufferView.
    // When not defined, accessor must be initialized with zeros;
    // `sparse` property or extensions could override zeros with actual values.
    BufferView* bufferView { nullptr };

    // [required] The datatype of components in the attribute.
    // All valid values correspond to WebGL enums.
    // The corresponding typed arrays are
    // `Int8Array`, `Uint8Array`, `Int16Array`, `Uint16Array`,
    // `Uint32Array`, and `Float32Array`, respectively.
    // 5125 (UNSIGNED_INT) is only allowed when the accessor contains indices,
    // i.e., the accessor is only referenced by `primitive.indices`.
    ComponentType componentType = ComponentType::UNSIGNED_INT;

    // [required] The number of attributes referenced by this accessor,
    // not to be confused with the number of bytes or number of components.
    // "minimum": 1
    uint32_t count = 0;

    // [required] Specifies if the attribute is a scalar, vector, or matrix.
    DataType type = DataType::INVALID;

    // The offset relative to the start of the bufferView in bytes.
    // This must be a multiple of the size of the component datatype.
    // minimum: 0
    // default: 0
    uint32_t byteOffset = 0;

    // Specifies whether integer data values should be normalized
    // (`true`) to [0, 1] (for unsigned types) or [-1, 1] (for signed types),
    // or converted directly (`false`) when they are accessed.
    // This property is defined only for accessors that contain vertex attributes
    // or animation output data.
    // "default": false
    bool normalized = false;

    // Maximum value of each component in this attribute.
    // Array elements must be treated as having the same data type as
    // accessor's `componentType`. Both min and max arrays have the same length.
    // The length is determined by the value of the type property;
    // it can be 1, 2, 3, 4, 9, or 16.
    // `normalized` property has no effect on array values:
    // they always correspond to the actual values stored in the buffer.
    // When accessor is sparse, this property must contain max values of
    // accessor data with sparse substitution applied.
    // "minItems": 1,
    // "maxItems": 16,
    BASE_NS::vector<float> max;

    // Minimum value of each component in this attribute.
    // Array elements must be treated as having the same data type as
    // accessor's `componentType`. Both min and max arrays have the same length.
    // The length is determined by the value of the type property;
    // it can be 1, 2, 3, 4, 9, or 16.
    // `normalized` property has no effect on array values:
    // they always correspond to the actual values stored in the buffer.
    // When accessor is sparse, this property must contain min values of
    // accessor data with sparse substitution applied.
    // "minItems": 1,
    // "maxItems": 16,
    BASE_NS::vector<float> min;

    // Sparse storage of attributes that deviate from their initialization value.
    Sparse sparse;
};

struct AttributeBase {
    AttributeType type;
    uint32_t index; // for example texcoord 0,1,2...
};

struct Attribute {
    AttributeBase attribute;
    Accessor* accessor { nullptr };
};

struct Image {
    // The uri of the image.
    // Relative paths are relative to the .gltf file.
    // Instead of referencing an external file,
    // the uri can also be a data-uri.
    // The image format must be jpg or png.
    BASE_NS::string uri;

    // The bufferView that contains the image.
    // Use this instead of the image's uri property.
    BufferView* bufferView { nullptr };

    // The image's MIME type. Needed when BufferView is used.
    MimeType type;
};

struct Sampler {
    FilterMode magFilter = FilterMode::LINEAR;
    FilterMode minFilter = FilterMode::LINEAR;

    WrapMode wrapS = WrapMode::REPEAT;
    WrapMode wrapT = WrapMode::REPEAT;
};

struct Texture {
    // The sampler used by this texture.
    // When nullptr, a sampler with repeat wrapping
    // and auto filtering should be used.
    Sampler* sampler { nullptr };

    // The image used by this texture.
    Image* image { nullptr };
};

struct TextureInfo {
    // The texture.
    Texture* texture { nullptr };

    // index defined in gltf.
    uint32_t index = GLTF_INVALID_INDEX;

    // The set index of texture's TEXCOORD attribute
    //  used for texture coordinate mapping.
    // "default": 0
    uint32_t texCoordIndex = 0;

#if defined(GLTF2_EXTENSION_KHR_TEXTURE_TRANSFORM)
    struct TextureTransform {
        BASE_NS::Math::Vec2 offset { 0.0f, 0.0f };
        BASE_NS::Math::Vec2 scale { 1.f, 1.f };
        float rotation = 0.0f;
        uint32_t texCoordIndex = GLTF_INVALID_INDEX;
    } transform;
#endif
};

struct MetallicRoughness {
    // The RGBA components of the base color of the material.
    // The fourth component (A) is the alpha coverage of the material.
    // The `alphaMode` property specifies how alpha is interpreted.
    //  These values are linear. If a baseColorTexture is specified,
    //  this value is multiplied with the texel values.
    //  "default": [ 1.0, 1.0, 1.0, 1.0 ]
    BASE_NS::Math::Vec4 baseColorFactor { 1.f, 1.f, 1.f, 1.f };

    // The base color texture.
    // This texture contains RGB(A) components in sRGB color space.
    // The first three components (RGB) specify the base color of the
    // material. If the fourth component (A) is present, it represents
    // the alpha coverage of the material. Otherwise, an alpha of 1.0 is
    // assumed. The `alphaMode` property specifies how alpha is
    // interpreted. The stored texels must not be premultiplied.
    // "default": 1.0
    TextureInfo baseColorTexture;

    // The metalness  of the material.
    // A value of 1.0 means the material is a metal.
    // A value of 0.0 means the material is a dielectric.
    // Values in between are for blending between metals
    // and dielectrics such as dirty metallic surfaces.
    // This value is linear. If a metallicRoughnessTexture is specified,
    // this value is multiplied with the metallic texel values.
    float metallicFactor { 1.f };

    // The roughness of the material.
    // A value of 1.0 means the material is completely rough.
    // A value of 0.0 means the material is completely smooth.
    // This value is linear. If a metallicRoughnessTexture is specified,
    // this value is multiplied with the roughness texel values.
    // "default": 1.0
    float roughnessFactor { 1.f };

    // The metallic-roughness texture.
    // The metalness values are sampled from the B channel.
    // The roughness values are sampled from the G channel.
    // These values are linear. If other channels are present (R or A),
    // they are ignored for metallic-roughness calculations.
    TextureInfo metallicRoughnessTexture;
};

struct NormalTexture {
    TextureInfo textureInfo;
    float scale = 1.0f;
};

struct OcclusionTexture {
    TextureInfo textureInfo;
    float strength = 1.0f;
};

struct Material {
    enum class Type { MetallicRoughness, SpecularGlossiness, Unlit, TextureSheetAnimation };

    Type type { Type::MetallicRoughness };

    BASE_NS::string name; // name
    MetallicRoughness metallicRoughness;

    // "The scalar multiplier applied to each normal vector of the texture.
    // This value scales the normal vector using the formula:
    // `scaledNormal =  normalize((normalize(<sampled normal texture value>) * 2.0
    // - 1.0) * vec3(<normal scale>, <normal scale>, 1.0))`. This value is ignored if
    // normalTexture is not specified. This value is linear."
    NormalTexture normalTexture;

    // A scalar multiplier controlling the amount of occlusion applied.
    // A value of 0.0 means no occlusion. A value of 1.0 means full occlusion.
    // This value affects the resulting color using the formula:
    // `occludedColor = lerp(color, color * <sampled occlusion texture value>,
    // <occlusion strength>)`. This value is ignored if the corresponding texture
    // is not specified. This value is linear. "default": 1.0, "minimum": 0.0,
    // "maximum": 1.0,
    OcclusionTexture occlusionTexture;

    TextureInfo emissiveTexture;

    BASE_NS::Math::Vec4 emissiveFactor = BASE_NS::Math::Vec4(0.f, 0.f, 0.f, 1.f); // "default": [ 0.0, 0.0, 0.0 ],

    AlphaMode alphaMode = AlphaMode::OPAQUE;

    BlendMode blendMode = BlendMode::NONE;

    float alphaCutoff = 0.5f; // "minimum": 0.0,

    // default": false,
    // Specifies whether the material is double sided.
    // When this value is false, back-face culling is enabled.
    // When this value is true, back-face culling is disabled
    // and double sided lighting is enabled.
    // The back-face must have its normals reversed before
    // the lighting equation is evaluated.
    bool doubleSided = false;

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_CLEARCOAT) || defined(GLTF2_EXTRAS_CLEAR_COAT_MATERIAL)
    struct Clearcoat {
        // The clearcoat layer intensity.
        float factor = 0.0f;
        // The clearcoat layer intensity texture.
        TextureInfo texture;
        // The clearcoat layer roughness.
        float roughness = 0.0f;
        // The clearcoat layer roughness texture.
        TextureInfo roughnessTexture;
        // The clearcoat normal map texture.
        NormalTexture normalTexture;
    } clearcoat;
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_IOR)
    struct Ior {
        // Material's index of refraction.
        float ior = 1.5f;
    } ior;
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_PBRSPECULARGLOSSINESS)
    struct SpecularGlossiness {
        // The RGBA components of the reflected diffuse color of the material.
        // Metals have a diffuse value of `[0.0, 0.0, 0.0]`. The fourth component (A) is the alpha coverage of the
        // material. The 'alphaMode' property specifies how alpha is interpreted. The values are linear.
        BASE_NS::Math::Vec4 diffuseFactor { 1.f, 1.f, 1.f, 1.f }; // "default": [ 1.0, 1.0, 1.0, 1.0 ]

        // The diffuse texture. This texture contains RGB(A) components of the reflected diffuse color of the material
        // in sRGB color space. If the fourth component (A) is present, it represents the alpha coverage of the
        // material. Otherwise, an alpha of 1.0 is assumed. The `alphaMode` property specifies how alpha is interpreted.
        // The stored texels must not be premultiplied.
        TextureInfo diffuseTexture;

        // The specular RGB color of the material. This value is linear.
        BASE_NS::Math::Vec3 specularFactor { 1.f, 1.f, 1.f }; // "default": [ 1.0, 1.0, 1.0 ]

        // The glossiness or smoothness of the material.A value of 1.0 means the material has full glossiness
        // or is perfectly smooth.A value of 0.0 means the material has no glossiness or is completely rough.This value
        // is linear.
        float glossinessFactor = 1.0f;

        // The specular-glossiness texture is RGBA texture, containing the specular color of the material (RGB
        // components) and its glossiness (A component). The values are in sRGB space.
        TextureInfo specularGlossinessTexture;
    } specularGlossiness;
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SHEEN)
    struct Sheen {
        // The sheen color in linear space
        BASE_NS::Math::Vec3 factor;
        // The sheen color (sRGB)
        TextureInfo texture;
        // The sheen roughness.
        float roughness = 0.0f;
        // The sheen roughness texture, stored in the alpha channel.
        TextureInfo roughnessTexture;
    } sheen;
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_SPECULAR)
    struct Specular {
        // The specular reflection strength.
        float factor = 1.f;
        // The specular reflection strength texture, stored in the alpha channel.
        TextureInfo texture;
        // The specular color in linear space.
        BASE_NS::Math::Vec3 color { 1.f, 1.f, 1.f };
        // The specular color texture. The values are in sRGB space.
        TextureInfo colorTexture;
    } specular;
#endif

#if defined(GLTF2_EXTENSION_KHR_MATERIALS_TRANSMISSION)
    struct Transmission {
        // Percentage of light that is transmitted through the surface
        float factor = 0.0f;
        // Transmission percentage of the surface, stored in the R channel. This will be multiplied by
        // transmissionFactor.
        TextureInfo texture;
    } transmission;
#endif
};

struct MorphTarget {
    // extension of spec
    // see https://github.com/KhronosGroup/glTF-Blender-Exporter/pull/153)
    //     https://github.com/KhronosGroup/glTF/issues/1036
    BASE_NS::string name;
    BASE_NS::vector<Attribute> target;
#if defined(GLTF2_EXTENSION_IGFX_COMPRESSED)
    // true when morph target is using IGFX_compressed extension.
    bool iGfxCompressed = false;
#endif
};

struct MeshPrimitive {
    // [required fields]
    // A dictionary object, where each key corresponds
    // to mesh attribute semantic and each value is the index
    // of the accessor containing attribute's data.
    BASE_NS::vector<Attribute> attributes;

    // "The index of the accessor that contains mesh indices.
    // When this is not defined, the primitives should be rendered
    // without indices using drawArrays.
    // When defined, the accessor must contain indices:
    // the `bufferView` referenced by the accessor should have
    // a `target` equal to 34963 (ELEMENT_ARRAY_BUFFER)
    // `componentType` must be 5121 (UNSIGNED_BYTE),
    // 5123 (UNSIGNED_SHORT) or 5125 (UNSIGNED_INT),
    // the latter may require enabling additional hardware support;
    // `type` must be `\"SCALAR\"`.
    // For triangle primitives, the front face has
    // a counter-clockwise (CCW) winding order."
    Accessor* indices { nullptr };

    // "The index of the material to apply to this primitive when rendering.
    Material* material { nullptr };

    // index defined in gltf.
    uint32_t materialIndex = GLTF_INVALID_INDEX;

    // The type of primitives to render. All valid values correspond to WebGL enums.
    RenderMode mode = RenderMode::TRIANGLES;

    // An array of Morph Targets,
    // each  Morph Target is a dictionary mapping attributes
    // (only `POSITION`, `NORMAL`, and `TANGENT` supported)
    // to their deviations in the Morph Target.
    BASE_NS::vector<MorphTarget> targets;
};

struct Mesh {
    // Name.
    BASE_NS::string name;
    // [required field], primitives
    BASE_NS::vector<MeshPrimitive> primitives;
    // Array of weights to be applied to the Morph Targets.
    BASE_NS::vector<float> weights;
};

struct Camera {
    // Name.
    BASE_NS::string name;

    CameraType type;

    union Attributes {
        struct Perspective {
            // PERSPECTIVE
            // minimum value for each is 0
            // => in this implementation negative is used to disable parameter
            float aspect;
            float yfov; // required
            float zfar;
            float znear; // required
        } perspective;

        struct Ortho {
            // ORTHOGRAPHIC
            // xmag, ymag cant't be zero
            // zfar, znear : minimum is zero
            // all are required
            float xmag;
            float ymag;
            float zfar;
            float znear;
        } ortho;
    } attributes;
};

struct Skin {
    BASE_NS::string name;

    // The accessor containing the floating-point 4X4 inverse-bind matrices.
    // The default is that each matrix is a 4X4 identity matrix,
    // which implies that inverse-bind matrices were pre-applied.
    Accessor* inverseBindMatrices { nullptr };

    // The node used as a skeleton root. When undefined, joints transforms resolve to scene root.
    Node* skeleton { nullptr };

    // The skeleton nodes, used as joints in this skin.
    BASE_NS::vector<Node*> joints;
};

struct Node {
    BASE_NS::string name;

    Mesh* mesh { nullptr };

    Camera* camera { nullptr };

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
    KHRLight* light { nullptr };
#endif

#if defined(GLTF2_EXTRAS_RSDZ)
    BASE_NS::string modelIdRSDZ;
#endif

    // Helpers mostly for skeleton support
    Node* parent { nullptr };
    bool isJoint = false;

    BASE_NS::vector<Node*> children;
    BASE_NS::vector<size_t> tmpChildren; // indices, used when gltf is parsed. (NOTE: move outside of node)

    Skin* skin { nullptr };
    uint32_t tmpSkin; // index to skin (NOTE: move outside of node)

    bool usesTRS = true;

    BASE_NS::Math::Vec3 translation { 0.f, 0.f, 0.f };
    BASE_NS::Math::Quat rotation { 0.f, 0.f, 0.f, 1.f };
    BASE_NS::Math::Vec3 scale { 1.f, 1.f, 1.f };

    BASE_NS::Math::Mat4X4 matrix;

    BASE_NS::vector<float> weights;
};

struct Scene {
    BASE_NS::string name;
    BASE_NS::vector<Node*> nodes;

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
    KHRLight* light { nullptr }; // Ambient light
#endif

#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
    size_t imageBasedLightIndex = GLTF_INVALID_INDEX;
#endif
};

struct AnimationSampler {
    Accessor* input { nullptr };
    Accessor* output { nullptr };
    AnimationInterpolation interpolation;
};

struct AnimationChannel // = animation.channel.target
{
    Node* node { nullptr };
    AnimationPath path;
};

struct AnimationTrack // = animation.channel
{
    AnimationChannel channel;
    AnimationSampler* sampler { nullptr };
};

struct Animation {
    BASE_NS::string name;
    BASE_NS::vector<AnimationTrack> tracks;
    BASE_NS::vector<BASE_NS::unique_ptr<AnimationSampler>> samplers;
};

// extensions
#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
struct KHRLight {
    BASE_NS::string name;
    LightType type = LightType::DIRECTIONAL;
    BASE_NS::Math::Vec3 color = BASE_NS::Math::Vec3(1.f, 1.f, 1.f); // RGB
    float intensity = 1.0f; // Intensity of the light source in lumens. default 1.0

    struct {
        float range = .0f;

        struct {
            // SPOT
            float innerAngle = 0.f;
            float outerAngle = 0.785398163397448f; // PI / 4
        } spot;

    } positional;

    struct {
        bool shadowCaster = false;
        float nearClipDistance = 100.f;
        float farClipDistance = 10000.f;
    } shadow;
};
#endif

#if defined(GLTF2_EXTENSION_EXT_LIGHTS_IMAGE_BASED)
struct ImageBasedLight {
    // Represents one mip level of a cube map.
    using CubemapMipLevel = BASE_NS::vector<size_t>;
    // Represents one set of irrandiance coefficients.
    using LightingCoeff = BASE_NS::vector<float>;

    // Name of the light.
    BASE_NS::string name;
    // Quaternion that represents the rotation of the IBL environment.
    BASE_NS::Math::Quat rotation { 0.0f, 0.0f, 0.0f, 1.0f };
    // Brightness multiplier for environment.
    float intensity { 1.0f };
    // Declares spherical harmonic coefficients for irradiance up to l=2. This is a 9x3 array.
    BASE_NS::vector<LightingCoeff> irradianceCoefficients;
    // Declares an array of the first N mips of the prefiltered cubemap.
    // Each mip is, in turn, defined with an array of 6 images, one for each cube face. i.e. this is an Nx6 array.
    BASE_NS::vector<CubemapMipLevel> specularImages;
    // The dimension (in pixels) of the first specular mip. This is needed to determine, pre-load, the total number of
    // mips needed.
    uint32_t specularImageSize { 0 };
    // Specular cubemap image, optional.
    size_t specularCubeImage { GLTF_INVALID_INDEX };
    // Skymap cubemap image, optional.
    size_t skymapImage { GLTF_INVALID_INDEX };
    // Skymap image lod level, optional.
    float skymapImageLodLevel { 0.0f };
};
#endif

} // namespace GLTF2
CORE3D_END_NAMESPACE()

#endif // CORE__GLTF__GLTF2_DATA_STRUCTURES_H