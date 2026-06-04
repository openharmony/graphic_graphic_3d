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

#include <scene/api/template/image_template.h>
#include <scene/interface/resource/image_info.h>
#include <scene/interface/resource/types.h>

#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/intf_object_registry.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class ImageTemplateTest : public ScenePluginTest {
public:
    META_NS::IObject::Ptr CreateTemplateObj() const
    {
        return META_NS::GetObjectRegistry().Create<META_NS::IObject>(ClassId::ImageTemplate);
    }

    static ImageLoadInfo MakeNonDefaultLoadInfo()
    {
        ImageLoadInfo info{};
        info.loadFlags = ImageLoadFlags::GENERATE_MIPS | ImageLoadFlags::FORCE_SRGB_BIT;
        info.info.usageFlags = ImageUsageFlag::SAMPLED_BIT | ImageUsageFlag::TRANSFER_DST_BIT;
        info.info.memoryFlags = MemoryPropertyFlag::DEVICE_LOCAL_BIT;
        info.info.creationFlags = EngineImageCreationFlag::GENERATE_MIPS;
        return info;
    }
};

/**
 * @tc.name: CreateImageTemplate
 * @tc.desc: ImageTemplate can be created via object registry
 * @tc.type: FUNC
 */
UNIT_TEST_F(ImageTemplateTest, CreateImageTemplate, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);
}

/**
 * @tc.name: PropertiesAccessible
 * @tc.desc: Static Name and ImageLoadInfo properties are accessible via API wrapper
 * @tc.type: FUNC
 */
UNIT_TEST_F(ImageTemplateTest, PropertiesAccessible, testing::ext::TestSize.Level1)
{
    ImageTemplate tmpl(CreateTemplateObj());
    ASSERT_TRUE(tmpl);

    EXPECT_TRUE(tmpl.Name());
    EXPECT_TRUE(tmpl.ImageLoadInfo());
}

/**
 * @tc.name: DefaultImageLoadInfo
 * @tc.desc: Newly created ImageTemplate has DEFAULT_IMAGE_LOAD_INFO
 * @tc.type: FUNC
 */
UNIT_TEST_F(ImageTemplateTest, DefaultImageLoadInfo, testing::ext::TestSize.Level1)
{
    ImageTemplate tmpl(CreateTemplateObj());
    ASSERT_TRUE(tmpl);

    const auto info = META_NS::GetValue(tmpl.ImageLoadInfo());
    EXPECT_EQ(info.loadFlags, DEFAULT_IMAGE_LOAD_INFO.loadFlags);
    EXPECT_EQ(info.info.usageFlags, DEFAULT_IMAGE_LOAD_INFO.info.usageFlags);
    EXPECT_EQ(info.info.memoryFlags, DEFAULT_IMAGE_LOAD_INFO.info.memoryFlags);
    EXPECT_EQ(info.info.creationFlags, DEFAULT_IMAGE_LOAD_INFO.info.creationFlags);
}

/**
 * @tc.name: NameProperty
 * @tc.desc: ImageTemplate supports INamed and GetName returns the Name property value
 * @tc.type: FUNC
 */
UNIT_TEST_F(ImageTemplateTest, NameProperty, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto named = interface_cast<META_NS::INamed>(obj.get());
    ASSERT_TRUE(named);

    META_NS::SetValue(named->Name(), BASE_NS::string("TestImage"));
    EXPECT_EQ(META_NS::GetValue(named->Name()), "TestImage");
}

/**
 * @tc.name: SetAndGetImageLoadInfo
 * @tc.desc: ImageLoadInfo values can be written and read back
 * @tc.type: FUNC
 */
UNIT_TEST_F(ImageTemplateTest, SetAndGetImageLoadInfo, testing::ext::TestSize.Level1)
{
    ImageTemplate tmpl(CreateTemplateObj());
    ASSERT_TRUE(tmpl);

    const auto expected = MakeNonDefaultLoadInfo();
    META_NS::SetValue(tmpl.ImageLoadInfo(), expected);

    const auto actual = META_NS::GetValue(tmpl.ImageLoadInfo());
    EXPECT_EQ(actual.loadFlags, expected.loadFlags);
    EXPECT_EQ(actual.info.usageFlags, expected.info.usageFlags);
    EXPECT_EQ(actual.info.memoryFlags, expected.info.memoryFlags);
    EXPECT_EQ(actual.info.creationFlags, expected.info.creationFlags);
}

/**
 * @tc.name: IResourceOptionsInterfaceCast
 * @tc.desc: ImageTemplate supports interface_cast to IResourceOptions so it can live in
 *           ResourceData::options
 * @tc.type: FUNC
 */
UNIT_TEST_F(ImageTemplateTest, IResourceOptionsInterfaceCast, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto options = interface_cast<CORE_NS::IResourceOptions>(obj.get());
    EXPECT_TRUE(options);
}

/**
 * @tc.name: MergeReturnsFalse
 * @tc.desc: Merge is intentionally a no-op that reports no change
 * @tc.type: FUNC
 */
UNIT_TEST_F(ImageTemplateTest, MergeReturnsFalse, testing::ext::TestSize.Level1)
{
    auto dst = CreateTemplateObj();
    auto src = CreateTemplateObj();
    ASSERT_TRUE(dst && src);

    auto dstOptions = interface_cast<CORE_NS::IResourceOptions>(dst.get());
    auto srcOptions = interface_cast<CORE_NS::IResourceOptions>(src.get());
    ASSERT_TRUE(dstOptions && srcOptions);

    EXPECT_FALSE(dstOptions->Merge(*srcOptions));
}

/**
 * @tc.name: CloneProducesIndependentCopy
 * @tc.desc: Clone produces an independent copy that carries Name and ImageLoadInfo values
 * @tc.type: FUNC
 */
UNIT_TEST_F(ImageTemplateTest, CloneProducesIndependentCopy, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto named = interface_cast<META_NS::INamed>(obj.get());
    ASSERT_TRUE(named);
    META_NS::SetValue(named->Name(), BASE_NS::string("OriginalImage"));

    ImageTemplate tmpl(obj);
    const auto expected = MakeNonDefaultLoadInfo();
    META_NS::SetValue(tmpl.ImageLoadInfo(), expected);

    auto options = interface_cast<CORE_NS::IResourceOptions>(obj.get());
    ASSERT_TRUE(options);

    auto cloneOptions = options->Clone();
    ASSERT_TRUE(cloneOptions);

    auto cloneNamed = interface_cast<META_NS::INamed>(cloneOptions.get());
    ASSERT_TRUE(cloneNamed);
    EXPECT_EQ(META_NS::GetValue(cloneNamed->Name()), "OriginalImage");

    auto cloneMeta = interface_cast<META_NS::IMetadata>(cloneOptions.get());
    ASSERT_TRUE(cloneMeta);
    auto cloneLoadInfo = cloneMeta->GetProperty<ImageLoadInfo>("ImageLoadInfo");
    ASSERT_TRUE(cloneLoadInfo);

    const auto cloneValue = META_NS::GetValue(cloneLoadInfo);
    EXPECT_EQ(cloneValue.loadFlags, expected.loadFlags);
    EXPECT_EQ(cloneValue.info.usageFlags, expected.info.usageFlags);

    // Verify independence: mutating clone does not affect original
    ImageLoadInfo changed = expected;
    changed.loadFlags = ImageLoadFlags::FLIP_VERTICALLY_BIT;
    META_NS::SetValue(cloneLoadInfo, changed);
    EXPECT_EQ(META_NS::GetValue(tmpl.ImageLoadInfo()).loadFlags, expected.loadFlags);
}

}  // namespace UTest
SCENE_END_NAMESPACE()
