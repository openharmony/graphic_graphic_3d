/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <core/io/intf_file_manager.h>
#include <core/json/json.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <render/render_data_structures.h>

#include "gltf/data.h"
#include "gltf/gltf2_util.h"
#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

/**
 * @tc.name: GetMimeTypeTest
 * @tc.desc: Tests for Get Mime Type Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, GetMimeTypeTest, testing::ext::TestSize.Level1)
{
    GLTF2::MimeType mimeType;
    EXPECT_TRUE(GLTF2::GetMimeType("image/jpeg", mimeType));
    EXPECT_EQ(GLTF2::MimeType::JPEG, mimeType);
    EXPECT_EQ("image/jpeg", GLTF2::GetMimeType(GLTF2::MimeType::JPEG));

    EXPECT_TRUE(GLTF2::GetMimeType("image/png", mimeType));
    EXPECT_EQ(GLTF2::MimeType::PNG, mimeType);
    EXPECT_EQ("image/png", GLTF2::GetMimeType(GLTF2::MimeType::PNG));

    EXPECT_TRUE(GLTF2::GetMimeType("image/ktx", mimeType));
    EXPECT_EQ(GLTF2::MimeType::KTX, mimeType);
    EXPECT_EQ("image/ktx", GLTF2::GetMimeType(GLTF2::MimeType::KTX));

    EXPECT_TRUE(GLTF2::GetMimeType("image/ktx2", mimeType));
    EXPECT_EQ(GLTF2::MimeType::KTX2, mimeType);
    EXPECT_EQ("image/ktx2", GLTF2::GetMimeType(GLTF2::MimeType::KTX2));

    EXPECT_TRUE(GLTF2::GetMimeType("image/vnd-ms.dds", mimeType));
    EXPECT_EQ(GLTF2::MimeType::DDS, mimeType);
    EXPECT_EQ("image/vnd-ms.dds", GLTF2::GetMimeType(GLTF2::MimeType::DDS));

    EXPECT_TRUE(GLTF2::GetMimeType("application/octet-stream", mimeType));
    EXPECT_EQ(GLTF2::MimeType::KTX, mimeType);

    EXPECT_FALSE(GLTF2::GetMimeType("image/invalid-type", mimeType));
    EXPECT_EQ(GLTF2::MimeType::INVALID, mimeType);
    EXPECT_EQ("INVALID", GLTF2::GetMimeType(GLTF2::MimeType::INVALID));
}

/**
 * @tc.name: GetDataTypeTest
 * @tc.desc: Tests for Get Data Type Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, GetDataTypeTest, testing::ext::TestSize.Level1)
{
    GLTF2::DataType dataType;
    EXPECT_TRUE(GLTF2::GetDataType("SCALAR", dataType));
    EXPECT_EQ(GLTF2::DataType::SCALAR, dataType);
    EXPECT_EQ("SCALAR", GLTF2::GetDataType(GLTF2::DataType::SCALAR));

    EXPECT_TRUE(GLTF2::GetDataType("VEC2", dataType));
    EXPECT_EQ(GLTF2::DataType::VEC2, dataType);
    EXPECT_EQ("VEC2", GLTF2::GetDataType(GLTF2::DataType::VEC2));

    EXPECT_TRUE(GLTF2::GetDataType("VEC3", dataType));
    EXPECT_EQ(GLTF2::DataType::VEC3, dataType);
    EXPECT_EQ("VEC3", GLTF2::GetDataType(GLTF2::DataType::VEC3));

    EXPECT_TRUE(GLTF2::GetDataType("VEC4", dataType));
    EXPECT_EQ(GLTF2::DataType::VEC4, dataType);
    EXPECT_EQ("VEC4", GLTF2::GetDataType(GLTF2::DataType::VEC4));

    EXPECT_TRUE(GLTF2::GetDataType("MAT2", dataType));
    EXPECT_EQ(GLTF2::DataType::MAT2, dataType);
    EXPECT_EQ("MAT2", GLTF2::GetDataType(GLTF2::DataType::MAT2));

    EXPECT_TRUE(GLTF2::GetDataType("MAT3", dataType));
    EXPECT_EQ(GLTF2::DataType::MAT3, dataType);
    EXPECT_EQ("MAT3", GLTF2::GetDataType(GLTF2::DataType::MAT3));

    EXPECT_TRUE(GLTF2::GetDataType("MAT4", dataType));
    EXPECT_EQ(GLTF2::DataType::MAT4, dataType);
    EXPECT_EQ("MAT4", GLTF2::GetDataType(GLTF2::DataType::MAT4));

    EXPECT_FALSE(GLTF2::GetDataType("INVALID-TYPE", dataType));
    EXPECT_EQ(GLTF2::DataType::INVALID, dataType);
    EXPECT_EQ("INVALID", GLTF2::GetDataType(GLTF2::DataType::INVALID));
}

/**
 * @tc.name: GetCameraTypeTest
 * @tc.desc: Tests for Get Camera Type Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, GetCameraTypeTest, testing::ext::TestSize.Level1)
{
    GLTF2::CameraType cameraType;
    EXPECT_TRUE(GLTF2::GetCameraType("perspective", cameraType));
    EXPECT_EQ(GLTF2::CameraType::PERSPECTIVE, cameraType);
    EXPECT_EQ("perspective", GLTF2::GetCameraType(GLTF2::CameraType::PERSPECTIVE));

    EXPECT_TRUE(GLTF2::GetCameraType("orthographic", cameraType));
    EXPECT_EQ(GLTF2::CameraType::ORTHOGRAPHIC, cameraType);
    EXPECT_EQ("orthographic", GLTF2::GetCameraType(GLTF2::CameraType::ORTHOGRAPHIC));

    EXPECT_FALSE(GLTF2::GetCameraType("invalid", cameraType));
    EXPECT_EQ(GLTF2::CameraType::INVALID, cameraType);
    EXPECT_EQ("INVALID", GLTF2::GetCameraType(GLTF2::CameraType::INVALID));
}

/**
 * @tc.name: GetAnimationInterpolationTest
 * @tc.desc: Tests for Get Animation Interpolation Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, GetAnimationInterpolationTest, testing::ext::TestSize.Level1)
{
    GLTF2::AnimationInterpolation animationInterpolation;
    EXPECT_TRUE(GLTF2::GetAnimationInterpolation("LINEAR", animationInterpolation));
    EXPECT_EQ(GLTF2::AnimationInterpolation::LINEAR, animationInterpolation);
    EXPECT_EQ("LINEAR", GLTF2::GetAnimationInterpolation(GLTF2::AnimationInterpolation::LINEAR));

    EXPECT_TRUE(GLTF2::GetAnimationInterpolation("STEP", animationInterpolation));
    EXPECT_EQ(GLTF2::AnimationInterpolation::STEP, animationInterpolation);
    EXPECT_EQ("STEP", GLTF2::GetAnimationInterpolation(GLTF2::AnimationInterpolation::STEP));

    EXPECT_TRUE(GLTF2::GetAnimationInterpolation("CUBICSPLINE", animationInterpolation));
    EXPECT_EQ(GLTF2::AnimationInterpolation::SPLINE, animationInterpolation);
    EXPECT_EQ("CUBICSPLINE", GLTF2::GetAnimationInterpolation(GLTF2::AnimationInterpolation::SPLINE));
}

/**
 * @tc.name: GetAnimationPathTest
 * @tc.desc: Tests for Get Animation Path Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, GetAnimationPathTest, testing::ext::TestSize.Level1)
{
    GLTF2::AnimationPath animationPath;
    EXPECT_TRUE(GLTF2::GetAnimationPath("translation", animationPath));
    EXPECT_EQ(GLTF2::AnimationPath::TRANSLATION, animationPath);
    EXPECT_EQ("translation", GLTF2::GetAnimationPath(GLTF2::AnimationPath::TRANSLATION));

    EXPECT_TRUE(GLTF2::GetAnimationPath("rotation", animationPath));
    EXPECT_EQ(GLTF2::AnimationPath::ROTATION, animationPath);
    EXPECT_EQ("rotation", GLTF2::GetAnimationPath(GLTF2::AnimationPath::ROTATION));

    EXPECT_TRUE(GLTF2::GetAnimationPath("scale", animationPath));
    EXPECT_EQ(GLTF2::AnimationPath::SCALE, animationPath);
    EXPECT_EQ("scale", GLTF2::GetAnimationPath(GLTF2::AnimationPath::SCALE));

    EXPECT_TRUE(GLTF2::GetAnimationPath("weights", animationPath));
    EXPECT_EQ(GLTF2::AnimationPath::WEIGHTS, animationPath);
    EXPECT_EQ("weights", GLTF2::GetAnimationPath(GLTF2::AnimationPath::WEIGHTS));

    EXPECT_TRUE(GLTF2::GetAnimationPath("visible", animationPath));
    EXPECT_EQ(GLTF2::AnimationPath::VISIBLE, animationPath);

    EXPECT_TRUE(GLTF2::GetAnimationPath("material.opacity", animationPath));
    EXPECT_EQ(GLTF2::AnimationPath::OPACITY, animationPath);

    EXPECT_FALSE(GLTF2::GetAnimationPath("invalid", animationPath));
    EXPECT_EQ("translation", GLTF2::GetAnimationPath(GLTF2::AnimationPath::INVALID));
}

#if defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)
/**
 * @tc.name: GetLightTypeTest
 * @tc.desc: Tests for Get Light Type Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, GetLightTypeTest, testing::ext::TestSize.Level1)
{
    GLTF2::LightType lightType;
    EXPECT_TRUE(GLTF2::GetLightType("directional", lightType));
    EXPECT_EQ(GLTF2::LightType::DIRECTIONAL, lightType);
    EXPECT_EQ("directional", GLTF2::GetLightType(GLTF2::LightType::DIRECTIONAL));

    EXPECT_TRUE(GLTF2::GetLightType("point", lightType));
    EXPECT_EQ(GLTF2::LightType::POINT, lightType);
    EXPECT_EQ("point", GLTF2::GetLightType(GLTF2::LightType::POINT));

    EXPECT_TRUE(GLTF2::GetLightType("spot", lightType));
    EXPECT_EQ(GLTF2::LightType::SPOT, lightType);
    EXPECT_EQ("spot", GLTF2::GetLightType(GLTF2::LightType::SPOT));

    EXPECT_TRUE(GLTF2::GetLightType("ambient", lightType));
    EXPECT_EQ(GLTF2::LightType::AMBIENT, lightType);
    EXPECT_EQ("ambient", GLTF2::GetLightType(GLTF2::LightType::AMBIENT));

    EXPECT_FALSE(GLTF2::GetLightType("invalid light", lightType));
    EXPECT_EQ(GLTF2::LightType::INVALID, lightType);
    EXPECT_EQ("INVALID", GLTF2::GetLightType(GLTF2::LightType::INVALID));
}
#endif // defined(GLTF2_EXTENSION_KHR_LIGHTS) || defined(GLTF2_EXTENSION_KHR_LIGHTS_PBR)

/**
 * @tc.name: GetAlphaModeTest
 * @tc.desc: Tests for Get Alpha Mode Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, GetAlphaModeTest, testing::ext::TestSize.Level1)
{
    GLTF2::AlphaMode alphaMode;
    EXPECT_TRUE(GLTF2::GetAlphaMode("BLEND", alphaMode));
    EXPECT_EQ(GLTF2::AlphaMode::BLEND, alphaMode);
    EXPECT_EQ("BLEND", GLTF2::GetAlphaMode(GLTF2::AlphaMode::BLEND));

    EXPECT_TRUE(GLTF2::GetAlphaMode("MASK", alphaMode));
    EXPECT_EQ(GLTF2::AlphaMode::MASK, alphaMode);
    EXPECT_EQ("MASK", GLTF2::GetAlphaMode(GLTF2::AlphaMode::MASK));

    EXPECT_TRUE(GLTF2::GetAlphaMode("OPAQUE", alphaMode));
    EXPECT_EQ(GLTF2::AlphaMode::OPAQUE, alphaMode);
    EXPECT_EQ("OPAQUE", GLTF2::GetAlphaMode(GLTF2::AlphaMode::OPAQUE));

    EXPECT_FALSE(GLTF2::GetAlphaMode("invalid alpha mode", alphaMode));
}

/**
 * @tc.name: ComponentAndDataTypesTest
 * @tc.desc: Tests for Component And Data Types Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, ComponentAndDataTypesTest, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(1u, GLTF2::GetComponentByteSize(GLTF2::ComponentType::BYTE));
    EXPECT_EQ(1u, GLTF2::GetComponentByteSize(GLTF2::ComponentType::UNSIGNED_BYTE));
    EXPECT_EQ(2u, GLTF2::GetComponentByteSize(GLTF2::ComponentType::SHORT));
    EXPECT_EQ(2u, GLTF2::GetComponentByteSize(GLTF2::ComponentType::UNSIGNED_SHORT));
    EXPECT_EQ(4u, GLTF2::GetComponentByteSize(GLTF2::ComponentType::INT));
    EXPECT_EQ(4u, GLTF2::GetComponentByteSize(GLTF2::ComponentType::UNSIGNED_INT));
    EXPECT_EQ(4u, GLTF2::GetComponentByteSize(GLTF2::ComponentType::FLOAT));
    EXPECT_EQ(0u, GLTF2::GetComponentByteSize(GLTF2::ComponentType::INVALID));

    EXPECT_EQ(1u, GLTF2::GetComponentsCount(GLTF2::DataType::SCALAR));
    EXPECT_EQ(2u, GLTF2::GetComponentsCount(GLTF2::DataType::VEC2));
    EXPECT_EQ(3u, GLTF2::GetComponentsCount(GLTF2::DataType::VEC3));
    EXPECT_EQ(4u, GLTF2::GetComponentsCount(GLTF2::DataType::VEC4));
    EXPECT_EQ(4u, GLTF2::GetComponentsCount(GLTF2::DataType::MAT2));
    EXPECT_EQ(9u, GLTF2::GetComponentsCount(GLTF2::DataType::MAT3));
    EXPECT_EQ(16u, GLTF2::GetComponentsCount(GLTF2::DataType::MAT4));
    EXPECT_EQ(0u, GLTF2::GetComponentsCount(GLTF2::DataType::INVALID));

    EXPECT_EQ(GLTF2::ComponentType::UNSIGNED_BYTE, GLTF2::GetAlternativeType(GLTF2::ComponentType::UNSIGNED_SHORT, 1u));
    EXPECT_EQ(GLTF2::ComponentType::UNSIGNED_SHORT, GLTF2::GetAlternativeType(GLTF2::ComponentType::UNSIGNED_INT, 2u));
    EXPECT_EQ(GLTF2::ComponentType::UNSIGNED_INT, GLTF2::GetAlternativeType(GLTF2::ComponentType::UNSIGNED_BYTE, 4u));
    EXPECT_EQ(GLTF2::ComponentType::BYTE, GLTF2::GetAlternativeType(GLTF2::ComponentType::SHORT, 1u));
    EXPECT_EQ(GLTF2::ComponentType::SHORT, GLTF2::GetAlternativeType(GLTF2::ComponentType::INT, 2u));
    EXPECT_EQ(GLTF2::ComponentType::INT, GLTF2::GetAlternativeType(GLTF2::ComponentType::BYTE, 4u));
    EXPECT_EQ(GLTF2::ComponentType::FLOAT, GLTF2::GetAlternativeType(GLTF2::ComponentType::FLOAT, 4u));

    EXPECT_EQ("COLOR_0", GLTF2::GetAttributeType(GLTF2::AttributeBase { GLTF2::AttributeType::COLOR, 0 }));
    EXPECT_EQ("INVALID", GLTF2::GetAttributeType(GLTF2::AttributeBase { GLTF2::AttributeType::INVALID, 0 }));
}

/**
 * @tc.name: ValidateTest
 * @tc.desc: Tests for Validate Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, ValidateTest, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(GLTF2::ValidatePrimitiveAttribute(
        GLTF2::AttributeType::COLOR, GLTF2::DataType::VEC2, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidatePrimitiveAttribute(
        GLTF2::AttributeType::COLOR, GLTF2::DataType::VEC4, GLTF2::ComponentType::SHORT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidatePrimitiveAttribute(
        GLTF2::AttributeType::INVALID, GLTF2::DataType::VEC4, GLTF2::ComponentType::SHORT)
                     .empty());
    EXPECT_TRUE(GLTF2::ValidatePrimitiveAttribute(
        GLTF2::AttributeType::COLOR, GLTF2::DataType::VEC4, GLTF2::ComponentType::FLOAT)
                    .empty());

    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttribute(
        GLTF2::AttributeType::NORMAL, GLTF2::DataType::MAT4, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttribute(
        GLTF2::AttributeType::NORMAL, GLTF2::DataType::VEC3, GLTF2::ComponentType::SHORT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttribute(
        GLTF2::AttributeType::POSITION, GLTF2::DataType::MAT4, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttribute(
        GLTF2::AttributeType::POSITION, GLTF2::DataType::VEC3, GLTF2::ComponentType::UNSIGNED_BYTE)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttribute(
        GLTF2::AttributeType::TANGENT, GLTF2::DataType::MAT4, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttribute(
        GLTF2::AttributeType::TANGENT, GLTF2::DataType::VEC3, GLTF2::ComponentType::UNSIGNED_BYTE)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttribute(
        GLTF2::AttributeType::JOINTS, GLTF2::DataType::VEC3, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_TRUE(GLTF2::ValidateMorphTargetAttribute(
        GLTF2::AttributeType::NORMAL, GLTF2::DataType::VEC3, GLTF2::ComponentType::FLOAT)
                    .empty());

#if defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
    EXPECT_FALSE(GLTF2::ValidatePrimitiveAttributeQuatization(
        GLTF2::AttributeType::COLOR, GLTF2::DataType::VEC2, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidatePrimitiveAttributeQuatization(
        GLTF2::AttributeType::COLOR, GLTF2::DataType::VEC4, GLTF2::ComponentType::SHORT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidatePrimitiveAttributeQuatization(
        GLTF2::AttributeType::INVALID, GLTF2::DataType::VEC4, GLTF2::ComponentType::SHORT)
                     .empty());
    EXPECT_TRUE(GLTF2::ValidatePrimitiveAttributeQuatization(
        GLTF2::AttributeType::COLOR, GLTF2::DataType::VEC4, GLTF2::ComponentType::FLOAT)
                    .empty());

    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttributeQuantization(
        GLTF2::AttributeType::NORMAL, GLTF2::DataType::MAT4, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttributeQuantization(
        GLTF2::AttributeType::NORMAL, GLTF2::DataType::VEC3, GLTF2::ComponentType::INT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttributeQuantization(
        GLTF2::AttributeType::POSITION, GLTF2::DataType::MAT4, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttributeQuantization(
        GLTF2::AttributeType::POSITION, GLTF2::DataType::VEC3, GLTF2::ComponentType::UNSIGNED_BYTE)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttributeQuantization(
        GLTF2::AttributeType::TANGENT, GLTF2::DataType::MAT4, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttributeQuantization(
        GLTF2::AttributeType::TANGENT, GLTF2::DataType::VEC3, GLTF2::ComponentType::UNSIGNED_BYTE)
                     .empty());
    EXPECT_FALSE(GLTF2::ValidateMorphTargetAttributeQuantization(
        GLTF2::AttributeType::JOINTS, GLTF2::DataType::VEC3, GLTF2::ComponentType::FLOAT)
                     .empty());
    EXPECT_TRUE(GLTF2::ValidateMorphTargetAttributeQuantization(
        GLTF2::AttributeType::NORMAL, GLTF2::DataType::VEC3, GLTF2::ComponentType::FLOAT)
                    .empty());
#endif // defined(GLTF2_EXTENSION_KHR_MESH_QUANTIZATION)
}

/**
 * @tc.name: SplitFilenameTest
 * @tc.desc: Tests for Split Filename Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, SplitFilenameTest, testing::ext::TestSize.Level1)
{
    {
        string_view base;
        string_view path;
        GLTF2::SplitFilename("something/abc/fn.txt", base, path);
        EXPECT_EQ("something/abc", path);
        EXPECT_EQ("fn.txt", base);
        string_view name;
        string_view extension;
        GLTF2::SplitBaseFilename(base, name, extension);
        EXPECT_EQ("fn", name);
        EXPECT_EQ("txt", extension);
    }
    {
        string_view base;
        string_view path;
        GLTF2::SplitFilename("file.obj", base, path);
        EXPECT_EQ("", path);
        EXPECT_EQ("file.obj", base);
        string_view name;
        string_view extension;
        GLTF2::SplitBaseFilename(base, name, extension);
        EXPECT_EQ("file", name);
        EXPECT_EQ("obj", extension);
    }
    {
        string_view base;
        string_view path;
        GLTF2::SplitFilename("noextension/file", base, path);
        EXPECT_EQ("noextension", path);
        EXPECT_EQ("file", base);
        string_view name;
        string_view extension;
        GLTF2::SplitBaseFilename(base, name, extension);
        EXPECT_EQ("file", name);
        EXPECT_EQ("", extension);
    }
}

/**
 * @tc.name: DataUriTest
 * @tc.desc: Tests for Data Uri Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, DataUriTest, testing::ext::TestSize.Level1)
{
    EXPECT_TRUE(GLTF2::IsDataURI("data:image/jpeg;base64,"));
    EXPECT_TRUE(GLTF2::IsDataURI("data:image/png;base64,"));
    EXPECT_TRUE(GLTF2::IsDataURI("data:image/bmp;base64,"));
    EXPECT_TRUE(GLTF2::IsDataURI("data:image/gif;base64,"));
    EXPECT_TRUE(GLTF2::IsDataURI("data:text/plain;base64,"));
    EXPECT_TRUE(GLTF2::IsDataURI("data:application/gltf-buffer;base64,"));
    EXPECT_TRUE(GLTF2::IsDataURI("data:application/octet-stream;base64,"));
    EXPECT_FALSE(GLTF2::IsDataURI("data:image/invalid;base64,"));
    EXPECT_FALSE(GLTF2::IsDataURI("dat:image/jpeg;base64,"));
    EXPECT_FALSE(GLTF2::IsDataURI("data:image/jpeg,"));
    EXPECT_FALSE(GLTF2::IsDataURI("data:image/jpeg;base64"));
    EXPECT_FALSE(GLTF2::IsDataURI("image/jpeg;base64,"));

    vector<uint8_t> data;
    EXPECT_TRUE(GLTF2::DecodeDataURI(data, "data:text/plain;base64,YWJjZA==", 4u, true));
    ASSERT_EQ(4, data.size());
    EXPECT_EQ(static_cast<uint8_t>('a'), data[0]);
    EXPECT_EQ(static_cast<uint8_t>('b'), data[1]);
    EXPECT_EQ(static_cast<uint8_t>('c'), data[2]);
    EXPECT_EQ(static_cast<uint8_t>('d'), data[3]);
}

/**
 * @tc.name: GltfDataTest
 * @tc.desc: Tests for Gltf Data Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, GltfDataTest, testing::ext::TestSize.Level1)
{
    auto& fileManager = UTest::GetTestContext()->engine->GetFileManager();
    GLTF2::Data data { fileManager };
    data.filepath = "test://image";

    data.buffers.push_back(unique_ptr<GLTF2::Buffer> { new GLTF2::Buffer {} });
    data.buffers.back()->byteLength = 4;
    data.buffers.back()->uri = "data:image/jpeg;base64,TFVNRQ==";

    data.bufferViews.push_back(unique_ptr<GLTF2::BufferView> { new GLTF2::BufferView {} });
    data.bufferViews.back()->buffer = data.buffers[0].get();
    data.bufferViews.back()->byteLength = 2u;
    data.bufferViews.back()->byteOffset = 2u;

    data.thumbnails.resize(4);
    data.thumbnails[0].extension = "png";
    data.thumbnails[0].uri = "thumbnailImage.png";
    data.thumbnails[1].extension = "png";
    data.thumbnails[1].uri = "data:image/png;base64,";
    data.thumbnails[2].extension = "png";
    data.thumbnails[2].uri = "data:image/png;base64,eHl6";
    data.thumbnails[3].extension = "png";
    data.thumbnails[3].uri = "data:text/plain;base64,eHl6";

    data.images.push_back(unique_ptr<GLTF2::Image> { new GLTF2::Image {} });
    data.images.back()->uri = "image.png";

    EXPECT_TRUE(data.LoadBuffers());
    data.ReleaseBuffers();

    data.buffers.push_back(unique_ptr<GLTF2::Buffer> { new GLTF2::Buffer {} });

    data.buffers.back()->byteLength = 4u;
    data.buffers.back()->uri = "data:image/jpeg;base64,";
    data.buffers.back()->data.clear();
    EXPECT_FALSE(data.LoadBuffers());

    // When file is smaller than byteLength, byteLength is clamped
    constexpr const uint32_t bigByteSize = 5000000u;
    data.buffers.back()->byteLength = bigByteSize;
    data.buffers.back()->uri = "canine_512x512.png";
    data.buffers.back()->data.clear();
    EXPECT_TRUE(data.LoadBuffers());
    EXPECT_GT(bigByteSize, data.buffers.back()->byteLength);

    // Load buffer from memory file
    data.memoryFile_ = fileManager.OpenFile("test://image/canine_512x512.png");
    data.buffers.back()->byteLength = bigByteSize;
    data.buffers.back()->uri = "";
    data.defaultResources = "test://image/canine_512x512.png";
    data.defaultResourcesOffset = -1;
    data.buffers.back()->data.clear();
    EXPECT_FALSE(data.LoadBuffers());
    data.defaultResourcesOffset = 0;
    EXPECT_TRUE(data.LoadBuffers());
    EXPECT_GT(bigByteSize, data.buffers.back()->byteLength);

    data.memoryFile_.reset();
    data.buffers.back()->byteLength = 4u;
    data.buffers.back()->uri = "nonExistingBuffer.bin";
    data.buffers.back()->data.clear();
    EXPECT_FALSE(data.LoadBuffers());

    EXPECT_EQ(3, data.GetExternalFileUris().size());
    EXPECT_EQ(CORE_GLTF_INVALID_INDEX, data.GetDefaultSceneIndex());
    EXPECT_EQ(0, data.GetSceneCount());
    EXPECT_EQ(4, data.GetThumbnailImageCount());
    EXPECT_EQ("", data.GetThumbnailImage(0).extension);
    EXPECT_EQ("", data.GetThumbnailImage(1).extension);
    EXPECT_EQ("png", data.GetThumbnailImage(2).extension);
    EXPECT_EQ("", data.GetThumbnailImage(3).extension);
    EXPECT_EQ("", data.GetThumbnailImage(5).extension);
}

/**
 * @tc.name: LoadDataTest
 * @tc.desc: Tests for Load Data Test. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GLTFUtilTest, LoadDataTest, testing::ext::TestSize.Level1)
{
    {
        GLTF2::Buffer buffer;
        buffer.byteLength = 16u;
        buffer.data.resize(buffer.byteLength);

        GLTF2::BufferView bufferView;
        bufferView.buffer = &buffer;
        bufferView.data = nullptr;

        GLTF2::Accessor accessor;
        accessor.bufferView = &bufferView;
        GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(accessor);
        EXPECT_FALSE(result.success);
    }
    {
        GLTF2::Buffer buffer;
        buffer.byteLength = 16u;
        buffer.data.resize(buffer.byteLength);

        GLTF2::BufferView bufferView;
        bufferView.buffer = &buffer;
        bufferView.data = buffer.data.data();
        bufferView.byteStride = 5u;

        GLTF2::Accessor accessor;
        accessor.bufferView = &bufferView;
        accessor.count = 5u;
        accessor.componentType = GLTF2::ComponentType::FLOAT;
        accessor.type = GLTF2::DataType::SCALAR;
        GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(accessor);
        EXPECT_FALSE(result.success);
    }
    {
        GLTF2::Buffer buffer;
        buffer.byteLength = 16u;
        buffer.data.resize(buffer.byteLength);

        GLTF2::BufferView bufferView;
        bufferView.buffer = &buffer;
        bufferView.data = buffer.data.data();
        bufferView.byteStride = 5u;

        GLTF2::Accessor accessor;
        accessor.bufferView = &bufferView;
        accessor.count = 5u;
        accessor.componentType = GLTF2::ComponentType::FLOAT;
        accessor.type = GLTF2::DataType::SCALAR;

        accessor.sparse.count = 1u;
        accessor.sparse.indices.bufferView = &bufferView;
        accessor.sparse.indices.byteOffset = 0u;
        accessor.sparse.indices.componentType = GLTF2::ComponentType::FLOAT;
        accessor.sparse.values.bufferView = &bufferView;
        accessor.sparse.values.byteOffset = 0u;

        GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(accessor);
        EXPECT_FALSE(result.success);
    }
    {
        GLTF2::Buffer buffer;
        buffer.byteLength = 16u;
        buffer.data.resize(buffer.byteLength);

        GLTF2::BufferView bufferView1;
        bufferView1.data = buffer.data.data();

        GLTF2::BufferView bufferView2;
        bufferView2.buffer = &buffer;
        bufferView2.data = buffer.data.data();

        GLTF2::Accessor accessor;
        accessor.bufferView = &bufferView1;
        accessor.count = 1u;

        accessor.sparse.count = 1u;
        accessor.sparse.indices.bufferView = &bufferView2;
        accessor.sparse.indices.byteOffset = 0u;
        accessor.sparse.indices.componentType = GLTF2::ComponentType::UNSIGNED_BYTE;
        accessor.sparse.values.bufferView = &bufferView2;
        accessor.sparse.values.byteOffset = 0u;

        GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(accessor);
        EXPECT_TRUE(result.success);
    }
    {
        GLTF2::Buffer buffer;
        buffer.byteLength = 16u;
        buffer.data.resize(buffer.byteLength);

        GLTF2::BufferView bufferView1;
        bufferView1.data = buffer.data.data();

        GLTF2::BufferView bufferView2;
        bufferView2.buffer = &buffer;
        bufferView2.data = buffer.data.data();

        GLTF2::Accessor accessor;
        accessor.bufferView = &bufferView1;

        accessor.sparse.count = 1u;
        accessor.sparse.indices.bufferView = &bufferView2;
        accessor.sparse.indices.byteOffset = 0u;
        accessor.sparse.indices.componentType = GLTF2::ComponentType::UNSIGNED_BYTE;
        accessor.sparse.values.bufferView = &bufferView2;
        accessor.sparse.values.byteOffset = 0u;

        GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(accessor);
        EXPECT_FALSE(result.success);
    }
    {
        GLTF2::Buffer buffer;
        buffer.byteLength = 16u;
        buffer.data.resize(buffer.byteLength);

        GLTF2::BufferView bufferView1;
        bufferView1.data = buffer.data.data();

        GLTF2::BufferView bufferView2;
        bufferView2.buffer = &buffer;
        bufferView2.data = buffer.data.data();

        GLTF2::Accessor accessor;
        accessor.bufferView = &bufferView1;

        accessor.sparse.count = 1u;
        accessor.sparse.indices.bufferView = &bufferView2;
        accessor.sparse.indices.byteOffset = 0u;
        accessor.sparse.indices.componentType = GLTF2::ComponentType::UNSIGNED_SHORT;
        accessor.sparse.values.bufferView = &bufferView2;
        accessor.sparse.values.byteOffset = 0u;

        GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(accessor);
        EXPECT_FALSE(result.success);
    }
    {
        GLTF2::Buffer buffer;
        buffer.byteLength = 16u;
        buffer.data.resize(buffer.byteLength);

        GLTF2::BufferView bufferView1;
        bufferView1.data = buffer.data.data();

        GLTF2::BufferView bufferView2;
        bufferView2.buffer = &buffer;
        bufferView2.data = buffer.data.data();

        GLTF2::Accessor accessor;
        accessor.bufferView = &bufferView1;

        accessor.sparse.count = 1u;
        accessor.sparse.indices.bufferView = &bufferView2;
        accessor.sparse.indices.byteOffset = 0u;
        accessor.sparse.indices.componentType = GLTF2::ComponentType::UNSIGNED_INT;
        accessor.sparse.values.bufferView = &bufferView2;
        accessor.sparse.values.byteOffset = 0u;

        GLTF2::GLTFLoadDataResult result = GLTF2::LoadData(accessor);
        EXPECT_FALSE(result.success);
    }
}
