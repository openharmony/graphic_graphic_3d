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

#include <scene/api/template/animation_template.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>
#include <scene/interface/template/intf_template_options.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_resource.h>

#include "scene/scene_test.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class AnimationTemplateTest : public ScenePluginTest {
public:
    META_NS::IObject::Ptr CreateTemplateObj() const
    {
        return META_NS::GetObjectRegistry().Create<META_NS::IObject>(ClassId::AnimationTemplate);
    }

    template<typename T>
    static void AddSetProperty(META_NS::IMetadata& meta, BASE_NS::string_view name, const T& value)
    {
        auto prop = META_NS::ConstructProperty<T>(name);
        meta.AddProperty(prop.GetProperty());
        META_NS::SetValue<T>(prop.GetProperty(), value);
    }

    static void AddCommonProperties(META_NS::IMetadata& meta, BASE_NS::string_view animType)
    {
        AddSetProperty(meta, "AnimationType", BASE_NS::string(animType));
        meta.AddProperty(META_NS::ConstructProperty<bool>("Enabled").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<int32_t>("LoopCount").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<float>("SpeedFactor").GetProperty());
    }

    static void AddTrackProperties(META_NS::IMetadata& meta)
    {
        AddCommonProperties(meta, "trackAnimation");
        meta.AddProperty(META_NS::ConstructProperty<float>("DurationMs").GetProperty());
        meta.AddProperty(META_NS::ConstructProperty<META_NS::IProperty::WeakPtr>("Property").GetProperty());
    }

    TrackAnimationTemplate CreateTrackTemplate() const
    {
        auto obj = CreateTemplateObj();
        if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
            AddTrackProperties(*meta);
        }
        return TrackAnimationTemplate(obj);
    }

    KeyframeAnimationTemplate CreateKeyframeTemplate() const
    {
        auto obj = CreateTemplateObj();
        if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
            AddCommonProperties(*meta, "keyframeAnimation");
            meta->AddProperty(META_NS::ConstructProperty<float>("DurationMs").GetProperty());
            meta->AddProperty(META_NS::ConstructProperty<META_NS::IProperty::WeakPtr>("Property").GetProperty());
        }
        return KeyframeAnimationTemplate(obj);
    }

    ContainerAnimationTemplate CreateContainerTemplate(BASE_NS::string_view type) const
    {
        auto obj = CreateTemplateObj();
        if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
            AddCommonProperties(*meta, type);
        }
        return ContainerAnimationTemplate(obj);
    }

    // Backward compat helper — returns AnimationTemplate (typedef for AnimationTemplateBase)
    AnimationTemplate CreateFullTemplate() const
    {
        auto obj = CreateTemplateObj();
        if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
            AddTrackProperties(*meta);
        }
        return AnimationTemplate(obj);
    }
};

/**
 * @tc.name: CreateAnimationTemplate
 * @tc.desc: AnimationTemplate can be created via object registry
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CreateAnimationTemplate, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);
}

/**
 * @tc.name: BasePropertiesAccessible
 * @tc.desc: Common animation template properties are accessible via base class
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, BasePropertiesAccessible, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_TRUE(tmpl.AnimationType());
    EXPECT_TRUE(tmpl.Enabled());
    EXPECT_TRUE(tmpl.LoopCount());
    EXPECT_TRUE(tmpl.SpeedFactor());
}

/**
 * @tc.name: TrackPropertiesAccessible
 * @tc.desc: Track-specific properties are accessible via TrackAnimationTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, TrackPropertiesAccessible, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateTrackTemplate();
    ASSERT_TRUE(tmpl);

    // Base properties
    EXPECT_TRUE(tmpl.AnimationType());
    EXPECT_TRUE(tmpl.Enabled());
    EXPECT_TRUE(tmpl.LoopCount());
    EXPECT_TRUE(tmpl.SpeedFactor());

    // Track-specific
    EXPECT_TRUE(tmpl.DurationMs());
    EXPECT_TRUE(tmpl.Property());
}

/**
 * @tc.name: KeyframePropertiesAccessible
 * @tc.desc: Keyframe-specific properties are accessible via KeyframeAnimationTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, KeyframePropertiesAccessible, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateKeyframeTemplate();
    ASSERT_TRUE(tmpl);

    // Base properties
    EXPECT_TRUE(tmpl.AnimationType());
    EXPECT_TRUE(tmpl.Enabled());

    // Keyframe-specific
    EXPECT_TRUE(tmpl.DurationMs());
    EXPECT_TRUE(tmpl.Property());
}

/**
 * @tc.name: ContainerPropertiesAccessible
 * @tc.desc: Container animation template provides Animations accessor
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, ContainerPropertiesAccessible, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateContainerTemplate("sequentialAnimation");
    ASSERT_TRUE(tmpl);

    // Base properties
    EXPECT_TRUE(tmpl.AnimationType());
    EXPECT_TRUE(tmpl.Enabled());

    // Animations not yet added, so accessor returns empty
    EXPECT_FALSE(tmpl.Animations());
}

/**
 * @tc.name: ContainerAnimationsArray
 * @tc.desc: Container template exposes Animations array after adding it
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, ContainerAnimationsArray, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);
    auto meta = interface_cast<META_NS::IMetadata>(obj.get());
    ASSERT_TRUE(meta);

    AddCommonProperties(*meta, "parallelAnimation");

    BASE_NS::vector<META_NS::IObject::Ptr> children;
    auto p = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("Animations", children);
    meta->AddProperty(p.GetProperty());

    auto tmpl = ContainerAnimationTemplate(obj);
    ASSERT_TRUE(tmpl.Animations());
    EXPECT_EQ(tmpl.Animations()->GetSize(), 0u);
}

/**
 * @tc.name: PropertiesDefaultNotSet
 * @tc.desc: Newly added properties (without value) are not marked as set
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, PropertiesDefaultNotSet, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateTrackTemplate();
    ASSERT_TRUE(tmpl);

    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.Enabled().GetProperty()));
    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.LoopCount().GetProperty()));
    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.SpeedFactor().GetProperty()));
    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.DurationMs().GetProperty()));
    EXPECT_FALSE(META_NS::IsValueSet(*tmpl.Property().GetProperty()));
}

/**
 * @tc.name: AddSetPropertyMarksAsSet
 * @tc.desc: Properties added via AddSetProperty are marked as set
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, AddSetPropertyMarksAsSet, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateFullTemplate();
    ASSERT_TRUE(tmpl);

    // AnimationType was added via AddSetProperty in AddTrackProperties
    EXPECT_TRUE(META_NS::IsValueSet(*tmpl.AnimationType().GetProperty()));
    EXPECT_EQ(META_NS::GetValue(tmpl.AnimationType()), "trackAnimation");
}

/**
 * @tc.name: NameProperty
 * @tc.desc: AnimationTemplate supports INamed and GetName returns the Name property value
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, NameProperty, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto named = interface_cast<META_NS::INamed>(obj.get());
    ASSERT_TRUE(named);

    META_NS::SetValue(named->Name(), BASE_NS::string("TestAnimation"));
    EXPECT_TRUE(META_NS::IsValueSet(*named->Name().GetProperty()));
    EXPECT_EQ(META_NS::GetValue(named->Name()), "TestAnimation");
    EXPECT_EQ(obj->GetName(), "TestAnimation");
}

/**
 * @tc.name: SetPropertyMarksAsSet
 * @tc.desc: Setting a property value marks it as set
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, SetPropertyMarksAsSet, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateTrackTemplate();
    ASSERT_TRUE(tmpl);

    META_NS::SetValue(tmpl.DurationMs(), 500.0f);
    EXPECT_TRUE(META_NS::IsValueSet(*tmpl.DurationMs().GetProperty()));
    EXPECT_EQ(META_NS::GetValue(tmpl.DurationMs()), 500.0f);
}

/**
 * @tc.name: CopyFromAnotherTemplate
 * @tc.desc: IResourceTemplate::CopyFrom copies set properties from another AnimationTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CopyFromAnotherTemplate, testing::ext::TestSize.Level1)
{
    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto m = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        AddTrackProperties(*m);
    }
    auto src = TrackAnimationTemplate(srcObj);
    META_NS::SetValue(src.DurationMs(), 2000.0f);
    META_NS::SetValue(src.SpeedFactor(), 1.5f);

    auto dst = CreateTrackTemplate();
    ASSERT_TRUE(dst);
    auto dstTempl = dst.GetPtr<IResourceTemplate>();
    ASSERT_TRUE(dstTempl);
    EXPECT_TRUE(dstTempl->CopyFrom(static_cast<const META_NS::IObject&>(*srcObj)));

    EXPECT_EQ(META_NS::GetValue(dst.DurationMs()), 2000.0f);
    EXPECT_EQ(META_NS::GetValue(dst.SpeedFactor()), 1.5f);
}

/**
 * @tc.name: CloneTemplate
 * @tc.desc: Clone produces an independent copy of the template
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CloneTemplate, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateTrackTemplate();
    ASSERT_TRUE(tmpl);

    auto named = interface_cast<META_NS::INamed>(tmpl.GetPtr().get());
    ASSERT_TRUE(named);
    META_NS::SetValue(named->Name(), BASE_NS::string("OriginalAnim"));
    META_NS::SetValue(tmpl.DurationMs(), 1000.0f);

    auto options = interface_cast<CORE_NS::IResourceOptions>(tmpl.GetPtr().get());
    ASSERT_TRUE(options);

    auto cloneOptions = options->Clone();
    ASSERT_TRUE(cloneOptions);

    auto cloneMeta = interface_cast<META_NS::IMetadata>(cloneOptions.get());
    ASSERT_TRUE(cloneMeta);

    auto cloneDuration = cloneMeta->GetProperty<float>("DurationMs");
    ASSERT_TRUE(cloneDuration);
    EXPECT_EQ(META_NS::GetValue(cloneDuration), 1000.0f);

    auto cloneNamed = interface_cast<META_NS::INamed>(cloneOptions.get());
    ASSERT_TRUE(cloneNamed);
    EXPECT_EQ(META_NS::GetValue(cloneNamed->Name()), "OriginalAnim");

    // Verify independence
    META_NS::SetValue(cloneDuration, 500.0f);
    EXPECT_EQ(META_NS::GetValue(tmpl.DurationMs()), 1000.0f);
}

/**
 * @tc.name: ClonePropagatesBaseAndTemplateContext
 * @tc.desc: Clone copies baseResource_ and templateContext_ along with property data
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, ClonePropagatesBaseAndTemplateContext, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto derived = interface_cast<META_NS::IDerivedResourceOptions>(obj.get());
    auto tc = interface_cast<ITemplateOptions>(obj.get());
    ASSERT_TRUE(derived && tc);

    CORE_NS::ResourceId base{"base-anim", "group-a"};
    derived->SetBaseResource(base);
    tc->SetTemplateContext(true);

    auto options = interface_cast<CORE_NS::IResourceOptions>(obj.get());
    ASSERT_TRUE(options);
    auto cloneOptions = options->Clone();
    ASSERT_TRUE(cloneOptions);

    auto cloneDerived = interface_cast<META_NS::IDerivedResourceOptions>(cloneOptions.get());
    auto cloneTc = interface_cast<ITemplateOptions>(cloneOptions.get());
    ASSERT_TRUE(cloneDerived && cloneTc);
    EXPECT_EQ(cloneDerived->GetBaseResource(), base);
    EXPECT_TRUE(cloneTc->IsTemplateContext());
}

/**
 * @tc.name: MergeCopiesOtherSetValues
 * @tc.desc: Merge promotes this's set values to defaults, then copies other's set values
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, MergeCopiesOtherSetValues, testing::ext::TestSize.Level1)
{
    auto dstObj = CreateTemplateObj();
    ASSERT_TRUE(dstObj);

    auto srcObj = CreateTemplateObj();
    ASSERT_TRUE(srcObj);
    if (auto srcMeta = interface_cast<META_NS::IMetadata>(srcObj.get())) {
        srcMeta->AddProperty(META_NS::ConstructProperty<float>("DurationMs").GetProperty());
    }
    auto src = TrackAnimationTemplate(srcObj);
    META_NS::SetValue(src.DurationMs(), 3000.0f);

    auto dstOptions = interface_cast<CORE_NS::IResourceOptions>(dstObj.get());
    auto srcOptions = interface_cast<CORE_NS::IResourceOptions>(srcObj.get());
    ASSERT_TRUE(dstOptions && srcOptions);

    EXPECT_TRUE(dstOptions->Merge(*srcOptions));

    auto dstMeta = interface_cast<META_NS::IMetadata>(dstObj.get());
    ASSERT_TRUE(dstMeta);
    auto dstDuration = dstMeta->GetProperty<float>("DurationMs");
    ASSERT_TRUE(dstDuration);
    EXPECT_TRUE(dstDuration.GetProperty()->IsValueSet());
    EXPECT_EQ(META_NS::GetValue(dstDuration), 3000.0f);
}

/**
 * @tc.name: MergePropagatesBaseResource
 * @tc.desc: Merge propagates a valid baseResource_ from source to destination
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, MergePropagatesBaseResource, testing::ext::TestSize.Level1)
{
    auto dst = CreateTemplateObj();
    auto src = CreateTemplateObj();
    ASSERT_TRUE(dst && src);

    auto srcDerived = interface_cast<META_NS::IDerivedResourceOptions>(src.get());
    ASSERT_TRUE(srcDerived);
    CORE_NS::ResourceId base{"base-anim", "group-a"};
    srcDerived->SetBaseResource(base);

    auto dstOptions = interface_cast<CORE_NS::IResourceOptions>(dst.get());
    auto srcOptions = interface_cast<CORE_NS::IResourceOptions>(src.get());
    ASSERT_TRUE(dstOptions && srcOptions);
    EXPECT_TRUE(dstOptions->Merge(*srcOptions));

    auto dstDerived = interface_cast<META_NS::IDerivedResourceOptions>(dst.get());
    ASSERT_TRUE(dstDerived);
    EXPECT_EQ(dstDerived->GetBaseResource(), base);
}

/**
 * @tc.name: IResourceTemplateInterfaceCast
 * @tc.desc: AnimationTemplate supports interface_cast to IResourceTemplate
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, IResourceTemplateInterfaceCast, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto rt = interface_cast<IResourceTemplate>(obj.get());
    EXPECT_TRUE(rt);
}

/**
 * @tc.name: ResourceType
 * @tc.desc: AnimationTemplate reports correct resource type
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, ResourceType, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto resource = interface_cast<CORE_NS::IResource>(obj.get());
    ASSERT_TRUE(resource);

    EXPECT_EQ(resource->GetResourceType(), ClassId::AnimationResourceTemplate.Id().ToUid());
}

/**
 * @tc.name: ArrayPropertyTimestamps
 * @tc.desc: TrackAnimationTemplate can hold array properties like Timestamps
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, ArrayPropertyTimestamps, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto meta = interface_cast<META_NS::IMetadata>(obj.get());
    ASSERT_TRUE(meta);

    AddTrackProperties(*meta);
    BASE_NS::vector<float> values = {0.0f, 0.25f, 0.5f, 1.0f};
    auto p = META_NS::ConstructArrayProperty<float>("Timestamps", values);
    meta->AddProperty(p.GetProperty());

    auto tmpl = TrackAnimationTemplate(obj);
    auto timestamps = tmpl.Timestamps();
    ASSERT_TRUE(timestamps);
    EXPECT_EQ(timestamps->GetSize(), 4u);
    EXPECT_EQ(timestamps->GetValueAt(0), 0.0f);
    EXPECT_EQ(timestamps->GetValueAt(3), 1.0f);
}

/**
 * @tc.name: ClonePreservesArrayProperties
 * @tc.desc: Clone preserves array property values
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, ClonePreservesArrayProperties, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto meta = interface_cast<META_NS::IMetadata>(obj.get());
    ASSERT_TRUE(meta);

    BASE_NS::vector<float> values = {0.0f, 0.5f, 1.0f};
    auto p = META_NS::ConstructArrayProperty<float>("Timestamps", values);
    meta->AddProperty(p.GetProperty());

    auto options = interface_cast<CORE_NS::IResourceOptions>(obj.get());
    ASSERT_TRUE(options);

    auto cloneOptions = options->Clone();
    ASSERT_TRUE(cloneOptions);

    auto cloneMeta = interface_cast<META_NS::IMetadata>(cloneOptions.get());
    ASSERT_TRUE(cloneMeta);

    auto cloneTimestamps = cloneMeta->GetArrayProperty<float>("Timestamps", META_NS::MetadataQuery::EXISTING);
    ASSERT_TRUE(cloneTimestamps);
    EXPECT_EQ(cloneTimestamps->GetSize(), 3u);
    EXPECT_EQ(cloneTimestamps->GetValueAt(1), 0.5f);
}

/**
 * @tc.name: MultiplePropertyTypes
 * @tc.desc: Template can hold mixed property types simultaneously
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, MultiplePropertyTypes, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateTrackTemplate();
    ASSERT_TRUE(tmpl);

    META_NS::SetValue(tmpl.DurationMs(), 1500.0f);
    META_NS::SetValue(tmpl.SpeedFactor(), 2.0f);
    META_NS::SetValue(tmpl.Enabled(), true);
    META_NS::SetValue(tmpl.LoopCount(), int32_t(3));

    EXPECT_EQ(META_NS::GetValue(tmpl.DurationMs()), 1500.0f);
    EXPECT_EQ(META_NS::GetValue(tmpl.SpeedFactor()), 2.0f);
    EXPECT_EQ(META_NS::GetValue(tmpl.Enabled()), true);
    EXPECT_EQ(META_NS::GetValue(tmpl.LoopCount()), 3);
}

/**
 * @tc.name: TrackKeyframeValuesArray
 * @tc.desc: TrackAnimationTemplate exposes KeyframeValues array accessor
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, TrackKeyframeValuesArray, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto meta = interface_cast<META_NS::IMetadata>(obj.get());
    ASSERT_TRUE(meta);

    AddTrackProperties(*meta);
    BASE_NS::vector<META_NS::IAny::Ptr> values;
    auto p = META_NS::ConstructArrayProperty<META_NS::IAny::Ptr>("KeyframeValues", values);
    meta->AddProperty(p.GetProperty());

    auto tmpl = TrackAnimationTemplate(obj);
    auto kfValues = tmpl.KeyframeValues();
    ASSERT_TRUE(kfValues);
    EXPECT_EQ(kfValues->GetSize(), 0u);
}

/**
 * @tc.name: KeyframeFromToProperties
 * @tc.desc: KeyframeAnimationTemplate exposes From and To accessors
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, KeyframeFromToProperties, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);

    auto meta = interface_cast<META_NS::IMetadata>(obj.get());
    ASSERT_TRUE(meta);

    AddCommonProperties(*meta, "keyframeAnimation");
    meta->AddProperty(META_NS::ConstructProperty<float>("DurationMs").GetProperty());
    meta->AddProperty(META_NS::ConstructProperty<META_NS::IAny::Ptr>("From").GetProperty());
    meta->AddProperty(META_NS::ConstructProperty<META_NS::IAny::Ptr>("To").GetProperty());

    auto tmpl = KeyframeAnimationTemplate(obj);
    EXPECT_TRUE(tmpl.From());
    EXPECT_TRUE(tmpl.To());
    EXPECT_TRUE(tmpl.DurationMs());
}

/**
 * @tc.name: SequentialContainerType
 * @tc.desc: ContainerAnimationTemplate works for sequential animation type
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, SequentialContainerType, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateContainerTemplate("sequentialAnimation");
    ASSERT_TRUE(tmpl);

    EXPECT_EQ(META_NS::GetValue(tmpl.AnimationType()), "sequentialAnimation");
}

/**
 * @tc.name: ParallelContainerType
 * @tc.desc: ContainerAnimationTemplate works for parallel animation type
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, ParallelContainerType, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateContainerTemplate("parallelAnimation");
    ASSERT_TRUE(tmpl);

    EXPECT_EQ(META_NS::GetValue(tmpl.AnimationType()), "parallelAnimation");
}

/**
 * @tc.name: CreateInstanceTrackAnimation
 * @tc.desc: CreateInstance produces a TrackAnimation for "trackAnimation" type
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CreateInstanceTrackAnimation, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateTrackTemplate();
    ASSERT_TRUE(tmpl);

    auto anim = tmpl.CreateInstance();
    ASSERT_TRUE(anim);

    auto obj = interface_cast<META_NS::IObject>(anim.get());
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetClassId(), META_NS::ClassId::TrackAnimation.Id());
}

/**
 * @tc.name: CreateInstanceKeyframeAnimation
 * @tc.desc: CreateInstance produces a KeyframeAnimation for "keyframeAnimation" type
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CreateInstanceKeyframeAnimation, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateKeyframeTemplate();
    ASSERT_TRUE(tmpl);

    auto anim = tmpl.CreateInstance();
    ASSERT_TRUE(anim);

    auto obj = interface_cast<META_NS::IObject>(anim.get());
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetClassId(), META_NS::ClassId::KeyframeAnimation.Id());
}

/**
 * @tc.name: CreateInstanceSequentialAnimation
 * @tc.desc: CreateInstance produces a SequentialAnimation for "sequentialAnimation" type
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CreateInstanceSequentialAnimation, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateContainerTemplate("sequentialAnimation");
    ASSERT_TRUE(tmpl);

    auto anim = tmpl.CreateInstance();
    ASSERT_TRUE(anim);

    auto obj = interface_cast<META_NS::IObject>(anim.get());
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetClassId(), META_NS::ClassId::SequentialAnimation.Id());
}

/**
 * @tc.name: CreateInstanceParallelAnimation
 * @tc.desc: CreateInstance produces a ParallelAnimation for "parallelAnimation" type
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CreateInstanceParallelAnimation, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateContainerTemplate("parallelAnimation");
    ASSERT_TRUE(tmpl);

    auto anim = tmpl.CreateInstance();
    ASSERT_TRUE(anim);

    auto obj = interface_cast<META_NS::IObject>(anim.get());
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetClassId(), META_NS::ClassId::ParallelAnimation.Id());
}

/**
 * @tc.name: CreateInstanceUnknownTypeReturnsNull
 * @tc.desc: CreateInstance returns nullptr for an unrecognised AnimationType string
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CreateInstanceUnknownTypeReturnsNull, testing::ext::TestSize.Level1)
{
    auto tmpl = CreateContainerTemplate("bogusAnimation");
    ASSERT_TRUE(tmpl);
    EXPECT_FALSE(tmpl.CreateInstance());
}

/**
 * @tc.name: CreateInstanceEmptyTypeReturnsNull
 * @tc.desc: CreateInstance returns nullptr when AnimationType is unset/empty
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CreateInstanceEmptyTypeReturnsNull, testing::ext::TestSize.Level1)
{
    auto obj = CreateTemplateObj();
    ASSERT_TRUE(obj);
    if (auto meta = interface_cast<META_NS::IMetadata>(obj.get())) {
        meta->AddProperty(META_NS::ConstructProperty<BASE_NS::string>("AnimationType").GetProperty());
    }
    auto tmpl = AnimationTemplate(obj);
    ASSERT_TRUE(tmpl);
    EXPECT_FALSE(tmpl.CreateInstance());
}

/**
 * @tc.name: CreateInstanceUninitializedWrapper
 * @tc.desc: CreateInstance on a wrapper that holds no object returns nullptr
 * @tc.type: FUNC
 */
UNIT_TEST_F(AnimationTemplateTest, CreateInstanceUninitializedWrapper, testing::ext::TestSize.Level1)
{
    AnimationTemplate tmpl(META_NS::IObject::Ptr{});
    EXPECT_FALSE(tmpl);
    EXPECT_FALSE(tmpl.CreateInstance());
}

}  // namespace UTest
SCENE_END_NAMESPACE()
