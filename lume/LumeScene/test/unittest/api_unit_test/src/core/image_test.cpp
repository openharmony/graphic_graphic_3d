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

#if __OHOS__
// #include <native_buffer/native_buffer.h>
#endif
#if __ANDROID__
#include <android/hardware_buffer.h>
#endif

#include <scene/interface/intf_image.h>
#include <scene/interface/resource/types.h>

#include <core/intf_engine.h>

#include "scene/scene_test.h"
#include "util.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ScenePluginImageTest : public ScenePluginTest {
public:
};

static void CopyImageFile(CORE_NS::IFileManager& manager, BASE_NS::string_view path, BASE_NS::string_view name)
{
    BASE_NS::string file = BASE_NS::string("file://image_reload/") + name;
    if (manager.FileExists(file)) {
        ASSERT_TRUE(manager.DeleteFile(file));
    }
    ASSERT_TRUE(CopyFile(manager, BASE_NS::string(path) + name, file));
}

#ifdef _WIN32
/**
 * @tc.name: ResourceReload
 * @tc.desc: Tests for Resource Reload. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginImageTest, ResourceReload, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: ResourceReload
 * @tc.desc: Tests for Resource Reload. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ScenePluginImageTest, DISABLED_ResourceReload, testing::ext::TestSize.Level1)
#endif
{
    CreateEmptyScene();

    auto& manager = context->GetRenderer()->GetEngine().GetFileManager();
    CopyImageFile(manager, "test://image_reload/1/", "image.png");

    EXPECT_TRUE(resources->AddResource("image", ClassId::ImageResource.Id().ToUid(), "file://image_reload/image.png"));
    auto image = interface_pointer_cast<IImage>(resources->GetResource("image"));
    ASSERT_TRUE(image);

    EXPECT_EQ(image->Size()->GetValue(), (BASE_NS::Math::UVec2 { 1200, 1219 }));

    CopyImageFile(manager, "test://image_reload/2/", "image.png");
    ASSERT_TRUE(resources->ReloadResource(interface_pointer_cast<CORE_NS::IResource>(image)));

    EXPECT_EQ(image->Size()->GetValue(), (BASE_NS::Math::UVec2 { 252, 256 }));
}

// #if defined(__ANDROID__)
// UNIT_TEST_F(API_ScenePluginImageTest, DISABLED_CreateExternalImage, testing::ext::TestSize.Level1)
// #else
// UNIT_TEST_F(API_ScenePluginImageTest, CreateExternalImage, testing::ext::TestSize.Level1)
// #endif
// {
// #if defined(__ANDROID__)
//    AHardwareBuffer_Desc desc { 4U, 4U, 1U, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
//        AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY | AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, 0U, 0U, 0U };
//    AHardwareBuffer* buf;
//    AHardwareBuffer_allocate(&desc, &buf);
//    ASSERT_TRUE(buf);

//     AHardwareBuffer_describe(buf, &desc);
// #elif defined(__OHOS__)
//     OH_NativeBuffer_Config config { 4, 4, OH_NativeBuffer_Format::NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
//         OH_NativeBuffer_Usage::NATIVEBUFFER_USAGE_HW_TEXTURE, 8 };
//     OH_NativeBuffer* buf = OH_NativeBuffer_Alloc(&config);
//     ASSERT_TRUE(buf);
//     OH_NativeBuffer_GetConfig(buf, &config);
// #else
//     char buf[1]; // This is not actually going anywhere
// #endif

//     scene = CreateEmptyScene();
//     auto ext = scene->CreateObject<IExternalImage>(ClassId::ExternalImage).GetResult();
//     ASSERT_TRUE(ext);
//     SCENE_NS::IExternalImage::ExternalBufferInfo buffer;

//     auto buffer_set = ext->SetExternalBuffer(buffer);

// #if defined(__ANDROID__)
//     ASSERT_TRUE(buffer_set);
//     AHardwareBuffer_release(buf);
// #elif defined(__OHOS__)
//     ASSERT_TRUE(buffer_set);
//     OH_NativeBuffer_Unreference(buf);
// #else
//     ASSERT_FALSE(buffer_set);
// #endif
// }

} // namespace UTest

SCENE_END_NAMESPACE()
