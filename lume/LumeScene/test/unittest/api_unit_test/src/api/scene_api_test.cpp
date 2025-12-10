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

#include <scene/api/ecs_scene.h>
#include <scene/api/node.h>
#include <scene/api/resource.h>
#include <scene/api/scene.h>
#include <scene/interface/intf_input_receiver.h>
#include <scene/interface/resource/types.h>

#include <gmock/gmock.h>

#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/transform_component.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <core/resources/intf_resource_manager.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_cache.h>
#include <render/device/intf_gpu_resource_manager.h>

#include <meta/api/animation.h>
#include <meta/api/curves.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/ext/attachment/behavior.h>

#include "scene/scene_test.h"

META_REGISTER_CLASS(
    ApiBehavior, "89a5468b-b600-4a84-a713-9a0197abd3bd", META_NS::ObjectCategoryBits::NO_CATEGORY, "Test behavior")

SCENE_BEGIN_NAMESPACE()
namespace UTest {

MATCHER_P(NameMatcher, name, "")
{
    return arg->GetName() == name;
}

class IApiBehavior : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IApiBehavior, "63e91165-72f9-4e9e-9be6-6b87a8e16f41")
public:
    virtual uint32_t GetInputEventDownCount() const = 0;
    virtual uint32_t GetInputEventUpCount() const = 0;
    virtual void SetPropagateEvent(bool propagate) = 0;
};

class ApiBehavior : public META_NS::Behavior<IApiBehavior, IInputReceiver> {
    META_OBJECT(ApiBehavior, ::ClassId::ApiBehavior, Behavior)
public:
protected: // IInputReceiver
    void OnInput(PointerEvent& event) override
    {
        if (event.pointers.size()) {
            if (event.pointers.front().state == PointerEvent::PointerState::POINTER_DOWN) {
                downCount_++;
            } else {
                upCount_++;
            }
        }
        event.handled = !propagate_;
    }

protected: // IApiBehavior
    uint32_t GetInputEventDownCount() const override
    {
        return downCount_;
    }
    uint32_t GetInputEventUpCount() const override
    {
        return upCount_;
    }
    void SetPropagateEvent(bool propagate) override
    {
        propagate_ = propagate;
    }

private:
    uint32_t downCount_ {};
    uint32_t upCount_ {};
    bool propagate_ { true };
};

class API_SceneApiTest : public ScenePluginTest {
    using Super = ScenePluginTest;
    void SetUp() override
    {
        Super::SetUp();

        auto& registry = META_NS::GetObjectRegistry();
        registry.RegisterObjectType(ApiBehavior::GetFactory());

        scene_ = Scene(CreateEmptyScene());
        ASSERT_TRUE(scene_);
        parent_ = scene_.GetResourceFactory().CreateNode("//root");
        ASSERT_TRUE(parent_);
    }

    void TearDown() override
    {
        scene_.Release();

        auto& registry = META_NS::GetObjectRegistry();
        registry.UnregisterObjectType(ApiBehavior::GetFactory());

        Super::TearDown();
    }

protected:
    Node& GetParent()
    {
        return parent_;
    }
    Scene& GetScene()
    {
        return scene_;
    }
    BASE_NS::string GetExpectedPath(BASE_NS::string_view nodeName)
    {
        return "//root/" + nodeName;
    }
    void CheckShader(Shader& shader)
    {
        EXPECT_TRUE(shader);
        EXPECT_EQ(shader.GetResourceType(), ClassId::ShaderResource.Id().ToUid());
    }

    static constexpr auto VALID_SHADER_URI = "test://shaders/test.shader";
    static constexpr auto INVALID_SHADER_URI = "test://this/is/a/nonexistent.shader";

private:
    Node parent_ { nullptr };
    Scene scene_ { nullptr };
};

using META_NS::Async;
using META_NS::CreateNew;
using META_NS::GetValue;
using META_NS::SetValue;

/**
 * @tc.name: InitializeScene
 * @tc.desc: Tests for Initialize Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, InitializeScene, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto root = scene.GetRootNode();
    auto rootAsync = Node(scene.GetRootNode<Async>().GetResult());

    EXPECT_TRUE(root);
    EXPECT_TRUE(rootAsync);
    EXPECT_EQ(root.GetPtr(), rootAsync.GetPtr());
}

/**
 * @tc.name: EcsAccess
 * @tc.desc: Tests for Ecs Access. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, EcsAccess, testing::ext::TestSize.Level1)
{
    auto scene = GetScene();
    auto ecsScene = EcsScene(scene);
    auto ecs = ecsScene.GetEcs();
    EXPECT_TRUE(ecs);
    auto ecso = EcsObject(scene.GetRootNode());
    EXPECT_TRUE(ecso);

    auto entity = ecso.GetEntity();
    EXPECT_TRUE(CORE_NS::EntityUtil::IsValid(entity));

    BASE_NS::vector<CORE_NS::IComponentManager*> components;
    auto success = ecsScene
                       .AddEcsThreadTask([&]() {
                           // Run in render thread
                           ecs->GetComponents(entity, components);
                           return !components.empty();
                       })
                       .GetResult();
    EXPECT_TRUE(success);
    EXPECT_FALSE(components.empty());
}

/**
 * @tc.name: SceneHierarchy
 * @tc.desc: Tests for Scene Hierarchy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, SceneHierarchy, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();

    auto root = scene.GetRootNode();
    EXPECT_TRUE(root);

    auto node = factory.CreateNode("//emptynode");
    EXPECT_TRUE(node);
    EXPECT_EQ(node, scene.GetNode("//emptynode"));
    EXPECT_EQ(node.GetPath(), "//emptynode");

    auto child = factory.CreateNode("child");
    EXPECT_TRUE(child);
    EXPECT_TRUE(node.AddChild(child));
    EXPECT_EQ(child.GetPath(), "//emptynode/child");

    EXPECT_TRUE(node.RemoveChild(child));
    EXPECT_EQ(child.GetPath(), "");
    scene.RemoveNode(node);
    EXPECT_FALSE(node);
}

/**
 * @tc.name: NodeTypes
 * @tc.desc: Tests for Node Types. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, NodeTypes, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode("//camera");
    auto light = factory.CreateLightNode("//light");

    ASSERT_TRUE(camera);
    ASSERT_TRUE(light);
    Node node = camera;
    ASSERT_TRUE(node);
    ASSERT_TRUE(camera);

    auto cameraAsLight = Light(interface_pointer_cast<CORE_NS::IInterface>(camera));
    EXPECT_FALSE(cameraAsLight);
    EXPECT_TRUE(camera);

    auto cameraAsNodeAsLight = Light(node);
    EXPECT_FALSE(cameraAsNodeAsLight);
    EXPECT_TRUE(camera);
    EXPECT_TRUE(node);

    auto cameraAsNodeAsCamera = Camera(node);
    EXPECT_TRUE(cameraAsNodeAsCamera);
}

/**
 * @tc.name: Find
 * @tc.desc: Tests for Find. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, Find, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto root = scene.GetRootNode();
    EXPECT_TRUE(root);
    auto parent = factory.CreateNode("//parent");
    EXPECT_TRUE(parent);
    auto child = factory.CreateNode("//parent/child");
    EXPECT_TRUE(child);

    EXPECT_EQ(scene.FindNode("child"), child);
    EXPECT_EQ(parent.FindNode("child"), child);
    EXPECT_EQ(scene.FindNode("parent").FindNode("child"), child);

    auto f = scene.FindNode<Async>("parent").Then(
        [](const INode::Ptr& node) { return Node(node).FindNode("child").GetPtr<INode>(); }, nullptr);
    EXPECT_EQ(f.GetResult(), child);
}

/**
 * @tc.name: FindNodes
 * @tc.desc: Tests for Find Nodes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, FindNodes, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto root = scene.GetRootNode();
    EXPECT_TRUE(root);
    auto parent = factory.CreateNode("//parent");
    EXPECT_TRUE(parent);
    auto child = factory.CreateNode("//parent/child");
    EXPECT_TRUE(child);
    auto childOfChild = factory.CreateNode("//parent/child/child");
    EXPECT_TRUE(childOfChild);
    auto childOfChildOfChild = factory.CreateNode("//parent/child/child/parent");
    EXPECT_TRUE(childOfChildOfChild);

    auto childs = scene.FindNodes("child", 0);
    auto parents = scene.FindNodes<META_NS::Async>("parent", 0).GetResult();
    EXPECT_THAT(childs, ::testing::ElementsAre(child, childOfChild));
    EXPECT_THAT(parents, ::testing::ElementsAre(parent, childOfChildOfChild));

    childs = scene.FindNodes("child", 1);
    parents = scene.FindNodes<META_NS::Async>("parent", 1).GetResult();
    EXPECT_THAT(childs, ::testing::ElementsAre(child));
    EXPECT_THAT(parents, ::testing::ElementsAre(parent));

    EXPECT_EQ(childOfChild.FindNode("parent"), childOfChildOfChild);
    childs = parent.FindNodes("child", 0);
    EXPECT_THAT(childs, ::testing::ElementsAre(child, childOfChild));
    auto n = Node(child.FindNode<META_NS::Async>("parent").GetResult());
    EXPECT_EQ(n, childOfChildOfChild);
}

/**
 * @tc.name: FindNodesTraversalOrder
 * @tc.desc: Tests for Find Nodes Traversal Order. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, FindNodesTraversalOrder, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto root = scene.GetRootNode();
    EXPECT_TRUE(root);
    auto l1_1 = factory.CreateNode("//first");
    auto l1_2 = factory.CreateNode("//second");
    auto l2_1 = factory.CreateNode("//first/first");
    auto l2_2 = factory.CreateNode("//second/first");
    auto l3_1 = factory.CreateNode("//first/first/first");

    auto firstsDepth = scene.FindNodes("first", 3, META_NS::TraversalType::FULL_HIERARCHY);
    auto firstsBreadth = scene.FindNodes("first", 3, META_NS::TraversalType::BREADTH_FIRST_ORDER);
    EXPECT_THAT(firstsDepth, ::testing::ElementsAre(l1_1, l2_1, l3_1));
    EXPECT_THAT(firstsBreadth, ::testing::ElementsAre(l1_1, l2_1, l2_2));
}

/**
 * @tc.name: GetComponent
 * @tc.desc: Tests for Get Component. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, GetComponent, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode("//camera");
    camera.SetFoV(42.f);
    auto component = scene.GetComponent(camera, "CameraComponent");
    EXPECT_EQ(component.GetName(), "CameraComponent");
    auto properties = META_NS::Metadata(component).GetProperties();
    EXPECT_THAT(properties, ::testing::Contains(NameMatcher("CameraComponent.yFov")));
    EXPECT_THAT(properties, ::testing::Contains(NameMatcher("CameraComponent.customProjectionMatrix")));
    auto fov = META_NS::Metadata(component).GetProperty<float>("CameraComponent.yFov");
    ASSERT_TRUE(fov);
    EXPECT_EQ(GetValue(fov), 42.f);
    SetValue(fov, 21.f);
    EXPECT_EQ(camera.GetFoV(), 21.f);
}

/**
 * @tc.name: CreateGenericComponent
 * @tc.desc: Tests for Create Generic Component. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, CreateGenericComponent, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode("//camera");
    ASSERT_TRUE(camera);
    // Create a new component (unknown to ScenePlugin)
    auto component = scene.CreateComponent(camera, "FogComponent");
    auto properties = META_NS::Metadata(component).GetProperties();
    EXPECT_EQ(properties.size(), 12);

    EXPECT_THAT(properties, ::testing::Contains(NameMatcher("FogComponent.density")));
    EXPECT_THAT(properties, ::testing::Contains(NameMatcher("FogComponent.additionalFactor")));

    auto inscatteringColor = component.GetProperty<BASE_NS::Math::Vec4>("inscatteringColor");
    ASSERT_TRUE(inscatteringColor);
    auto value = META_NS::GetValue(inscatteringColor);
    EXPECT_EQ(value.x, 1.f);
    EXPECT_EQ(value.y, 1.f);
    EXPECT_EQ(value.z, 1.f);
    EXPECT_EQ(value.w, 1.f);

    auto getter = scene.GetComponent(camera, "FogComponent");
    EXPECT_EQ(getter, component);
}

/**
 * @tc.name: CreateComponent
 * @tc.desc: Tests for Create Component. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, CreateComponent, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode("//camera");
    ASSERT_TRUE(camera);
    // Create a new component (unknown to ScenePlugin)
    auto component = scene.CreateComponent(camera, "LightComponent");
    auto properties = META_NS::Metadata(component).GetProperties();
    EXPECT_EQ(properties.size(), 14);

    EXPECT_THAT(properties, ::testing::Contains(NameMatcher("LightComponent.type")));
    EXPECT_THAT(properties, ::testing::Contains(NameMatcher("LightComponent.shadowLayerMask")));

    auto color = META_NS::Metadata(component).GetProperty<BASE_NS::Math::Vec3>("LightComponent.color");
    ASSERT_TRUE(color);
    auto value = META_NS::GetValue(color);
    EXPECT_EQ(value.x, 1.f);
    EXPECT_EQ(value.y, 1.f);
    EXPECT_EQ(value.z, 1.f);

    auto getter = scene.GetComponent(camera, "LightComponent");
    EXPECT_EQ(getter, component);
}

/**
 * @tc.name: GetRenderConfiguration
 * @tc.desc: Tests for Get Render Configuration. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, GetRenderConfiguration, testing::ext::TestSize.Level1)
{
    auto scene = interface_pointer_cast<IScene>(GetScene());
    ASSERT_TRUE(scene);
    auto src = scene->RenderConfiguration();
    auto rc = META_NS::GetValue(src);
    ASSERT_TRUE(rc);
    auto rca = interface_pointer_cast<IRenderConfiguration>(
        scene->CreateComponent(scene->GetRootNode().GetResult(), "RenderConfigurationComponent").GetResult());
    ASSERT_TRUE(rca);
    EXPECT_EQ(rc, rca);
}

/**
 * @tc.name: CreateRenderConfigurationComponent
 * @tc.desc: Tests for Create Render Configuration Component. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, CreateRenderConfigurationComponent, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto root = scene.GetRootNode();
    ASSERT_TRUE(root);
    auto component = scene.CreateComponent(root, "RenderConfigurationComponent");
    ASSERT_TRUE(component);
    auto meta = META_NS::Metadata(component);
    auto typeP = meta.GetProperty<SceneShadowType>("ShadowType");
    auto qualityP = meta.GetProperty<SceneShadowQuality>("ShadowQuality");
    auto smoothnessP = meta.GetProperty<SceneShadowSmoothness>("ShadowSmoothness");

    ASSERT_TRUE(typeP);
    ASSERT_TRUE(qualityP);
    ASSERT_TRUE(smoothnessP);

    // Default values from engine side at the time of writing the test
    EXPECT_EQ(META_NS::GetValue(typeP), SceneShadowType::PCF);
    EXPECT_EQ(META_NS::GetValue(qualityP), SceneShadowQuality::NORMAL);
    EXPECT_EQ(META_NS::GetValue(smoothnessP), SceneShadowSmoothness::NORMAL);
}

/**
 * @tc.name: Camera
 * @tc.desc: Tests for Camera. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, Camera, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto camera = factory.CreateCameraNode("camera");
    EXPECT_EQ(camera.GetClassId(), ClassId::CameraNode);
    EXPECT_TRUE(GetParent().AddChild(camera));
    EXPECT_EQ(camera.GetPath(), GetExpectedPath("camera"));
}

/**
 * @tc.name: Light
 * @tc.desc: Tests for Light. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, Light, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto light = factory.CreateLightNode("light");
    EXPECT_EQ(light.GetClassId(), ClassId::LightNode);
    EXPECT_TRUE(GetParent().AddChild(light));
    EXPECT_EQ(light.GetPath(), GetExpectedPath("light"));
}

/**
 * @tc.name: Node
 * @tc.desc: Tests for Node. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, Node, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto node = factory.CreateNode("node");
    EXPECT_EQ(node.GetClassId(), ClassId::Node);
    EXPECT_TRUE(GetParent().AddChild(node));
    EXPECT_EQ(node.GetPath(), GetExpectedPath("node"));
}

/**
 * @tc.name: LayerMask
 * @tc.desc: Tests for Layer Mask. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, LayerMask, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto node = factory.CreateNode("node");
    EcsScene ecss(scene);
    EcsObject ecso(node);
    ASSERT_TRUE(ecss);
    ASSERT_TRUE(ecso);
    auto entity = ecso.GetEntity();
    auto getLayerMask = [&]() {
        auto lcm = static_cast<CORE3D_NS::ILayerComponentManager*>(
            ecss.GetEcs()->GetComponentManager(CORE3D_NS::ILayerComponentManager::UID));
        return lcm->Get(entity).layerMask;
    };

    auto ecsMask = getLayerMask();
    auto mask = node.GetLayerMask();
    EXPECT_EQ(mask, ecsMask);
}

/**
 * @tc.name: NodeTransform
 * @tc.desc: Tests for Node Transform. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, NodeTransform, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto node = factory.CreateNode("node");
    EXPECT_EQ(node.GetClassId(), ClassId::Node);
    EXPECT_TRUE(GetParent().AddChild(node));
    EXPECT_EQ(node.GetPath(), GetExpectedPath("node"));
    auto mc = scene.GetComponent(node, "WorldMatrixComponent");
    EXPECT_TRUE(mc);
    auto transform = mc.GetProperty<BASE_NS::Math::Mat4X4>("matrix");
    ASSERT_TRUE(mc);

    uint32_t changed {};
    META_NS::PropertyChangedEventHandler h;
    h.Subscribe(transform, [&]() { changed++; });

    auto quat = BASE_NS::Math::Euler(45.f, 45.f, 45.f);
    // Set rotation on root node of our scene
    scene.GetRootNode().SetRotation(quat);
    EXPECT_EQ(changed, 0); // No change event yet as scene hasn't been updated
    UpdateScene();
    EXPECT_EQ(changed, 1); // Should have received a change event for the property when scene was updated

    // Get the TRS of our child node, it has no transform of its own so such now match the root
    // node's oritentation we set above
    [[maybe_unused]] BASE_NS::Math::Vec3 scale;
    BASE_NS::Math::Quat orientation;
    [[maybe_unused]] BASE_NS::Math::Vec3 translation;
    EXPECT_TRUE(BASE_NS::Math::Decompose(META_NS::GetValue(transform), scale, orientation, translation));
    EXPECT_EQ(orientation, quat);
}

/**
 * @tc.name: LoadShader
 * @tc.desc: Tests for Load Shader. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, LoadShader, testing::ext::TestSize.Level1)
{
    auto factory = GetScene().GetResourceFactory();
    auto shader = factory.CreateShader(VALID_SHADER_URI);
    CheckShader(shader);
    auto invalid = factory.CreateShader(INVALID_SHADER_URI);
    EXPECT_FALSE(invalid);
}

/**
 * @tc.name: LoadShaderAsync
 * @tc.desc: Tests for Load Shader Async. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, LoadShaderAsync, testing::ext::TestSize.Level1)
{
    auto factory = GetScene().GetResourceFactory();
    auto shader = Shader(factory.CreateShader<Async>(VALID_SHADER_URI).GetResult());
    CheckShader(shader);
    auto invalid = Shader(factory.CreateShader<Async>(INVALID_SHADER_URI).GetResult());
    EXPECT_FALSE(invalid);
}

/**
 * @tc.name: MetallicRoughnessMaterial
 * @tc.desc: Tests for Metallic Roughness Material. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, MetallicRoughnessMaterial, testing::ext::TestSize.Level1)
{
    auto factory = GetScene().GetResourceFactory();
    auto material = MetallicRoughnessMaterial(factory.CreateMaterial({ "material", MaterialType::METALLIC_ROUGHNESS }));
    ASSERT_TRUE(material);
    EXPECT_EQ(material.GetType(), MaterialType::METALLIC_ROUGHNESS);

    // Set base color factor
    auto color = BASE_NS::MakeColorFromLinear(0xffff0000);
    material.GetBaseColor().SetFactor(color);

    // Access BASE_COLOR texture slot and check that the property value matches what we just setl
    auto ptr = material.GetPtr<IMaterial>();
    ASSERT_TRUE(ptr);
    auto textures = ptr->Textures();
    ASSERT_TRUE(textures);
    auto texture = textures->GetValueAt(CORE3D_NS::MaterialComponent::TextureIndex::BASE_COLOR);
    ASSERT_TRUE(texture);
    EXPECT_EQ(GetValue(texture->Factor()), color);
    EXPECT_EQ(texture, material.GetMaterialPropertyAt(MetallicRoughnessMaterial::MaterialPropertyIndex::BASE_COLOR));
}

/**
 * @tc.name: Sampler
 * @tc.desc: Tests for Sampler. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, Sampler, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto factory = scene.GetResourceFactory();
    auto material = MetallicRoughnessMaterial(factory.CreateMaterial({ "material", MaterialType::METALLIC_ROUGHNESS }));
    ASSERT_TRUE(material);
    EXPECT_EQ(material.GetType(), MaterialType::METALLIC_ROUGHNESS);

    EcsScene ecss(scene);
    EcsObject ecso(material);
    ASSERT_TRUE(ecss);
    ASSERT_TRUE(ecso);

    auto entity = ecso.GetEntity();
    auto sampler = material.GetBaseColor().GetSampler();

    EXPECT_EQ(sampler.GetMinFilter(), SamplerFilter::LINEAR);
    EXPECT_EQ(sampler.GetMagFilter(), SamplerFilter::LINEAR);
    UpdateScene();

    auto getSampler = [&]() {
        auto mcm = static_cast<CORE3D_NS::IMaterialComponentManager*>(
            ecss.GetEcs()->GetComponentManager(CORE3D_NS::IMaterialComponentManager::UID));
        return mcm->Get(entity).textures[CORE3D_NS::MaterialComponent::TextureIndex::BASE_COLOR].sampler;
    };

    auto getSamplerDescriptor = [&]() {
        RENDER_NS::RenderHandleReference rhr;
        auto sampler = getSampler();
        auto iss = scene.GetPtr<IScene>()->GetInternalScene();
        if (auto rhman = static_cast<CORE3D_NS::IRenderHandleComponentManager*>(
                iss->GetEcsContext().FindComponent<CORE3D_NS::RenderHandleComponent>())) {
            rhr = rhman->GetRenderHandleReference(sampler);
        }
        return iss->GetRenderContext().GetDevice().GetGpuResourceManager().GetSamplerDescriptor(rhr);
    };
    // Should be null at this point since we have not set anything
    EXPECT_EQ(getSampler(), CORE_NS::EntityReference {});

    // Set mag filter, this should cause a new RenderHandleReference to be created and therefore a valid
    // EntityReference to be set in the material
    sampler.SetMagFilter(SamplerFilter::NEAREST);
    UpdateScene();
    EXPECT_NE(getSampler(), CORE_NS::EntityReference {});
    EXPECT_EQ(getSamplerDescriptor().magFilter, RENDER_NS::Filter::CORE_FILTER_NEAREST);

    // Reset the sampler, should return back to null EntityReference
    sampler.ResetObject();
    UpdateScene();
    EXPECT_EQ(getSampler(), CORE_NS::EntityReference {});
    EXPECT_EQ(sampler.GetMagFilter(), SamplerFilter::LINEAR);

    // Set sampler to non-default, should create a new sampler
    sampler.SetMagFilter(SamplerFilter::NEAREST);
    UpdateScene();
    EXPECT_NE(getSampler(), CORE_NS::EntityReference {});
    EXPECT_EQ(getSamplerDescriptor().magFilter, RENDER_NS::Filter::CORE_FILTER_NEAREST);
    // Set values to match the default sampler, should end up as null in ecs
    sampler.SetMagFilter(SamplerFilter::LINEAR);
    UpdateScene();
    EXPECT_EQ(getSampler(), CORE_NS::EntityReference {});
}

/**
 * @tc.name: CustomMaterial
 * @tc.desc: Tests for Custom Material. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, CustomMaterial, testing::ext::TestSize.Level1)
{
    auto factory = GetScene().GetResourceFactory();
    auto material = factory.CreateMaterial({ "material", MaterialType::CUSTOM });
    ASSERT_TRUE(material);
    EXPECT_EQ(material.GetType(), MaterialType::CUSTOM);
    EXPECT_TRUE(material.SetMaterialShader(factory.CreateShader(VALID_SHADER_URI)));
    EXPECT_NE(material.GetMaterialShader(), Shader(nullptr));

    auto customProp = material.GetCustomProperty<float>("f_");
    ASSERT_TRUE(customProp);
    customProp->SetValue(42.f);
    EXPECT_EQ(customProp->GetValue(), 42.f);

    auto customProps = material.GetCustomProperties().GetProperties();
    ASSERT_EQ(customProps.size(), 4);
    EXPECT_EQ(customProps[0]->GetName(), "v4_");
    EXPECT_EQ(customProps[1]->GetName(), "uv4_");
    EXPECT_EQ(customProps[2]->GetName(), "f_");
    EXPECT_EQ(customProps[3]->GetName(), "u_");

    auto p = META_NS::Property<float>(customProps[2]);
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetProperty(), customProp.GetProperty().get());
    EXPECT_EQ(p->GetValue(), 42.f);
}

/**
 * @tc.name: SwitchMaterialShader
 * @tc.desc: Tests for Switch Material Shader. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, SwitchMaterialShader, testing::ext::TestSize.Level1)
{
    auto factory = GetScene().GetResourceFactory();
    auto metallicRoughnessMaterial =
        MetallicRoughnessMaterial(factory.CreateMaterial({ "material", MaterialType::METALLIC_ROUGHNESS }));
    auto shader = factory.CreateShader("test://shaders/Custom.shader");
    EXPECT_TRUE(shader);
    EXPECT_TRUE(metallicRoughnessMaterial);

    size_t shaderChangedCount = 0;
    auto texturesProp = metallicRoughnessMaterial.GetPtr<IMaterial>()->Textures();
    texturesProp->OnChanged()->AddHandler(
        META_NS::MakeCallback<META_NS::IOnChanged>([&shaderChangedCount]() { shaderChangedCount++; }));

    EXPECT_TRUE(metallicRoughnessMaterial.SetMaterialShader(shader));

    UpdateScene();
    EXPECT_EQ(shaderChangedCount, 1);

    auto material = ShaderMaterial(metallicRoughnessMaterial);
    auto textures = material.GetMaterialProperties();
    ASSERT_EQ(textures.size(), 11);
    EXPECT_EQ(textures[0].GetName(), "Albedo");
    EXPECT_EQ(textures[0], material.GetMaterialProperty("Albedo"));
    auto customProperties = material.GetCustomProperties().GetProperties();
    ASSERT_EQ(customProperties.size(), 1);
    auto albedo = META_NS::Property<BASE_NS::Math::Vec4>(customProperties[0]);
    ASSERT_TRUE(albedo);
    EXPECT_EQ(albedo->GetName(), "AlbedoFactor");
    EXPECT_EQ(albedo->GetValue(), BASE_NS::Math::Vec4(1, 1, 0, 1));

    EXPECT_EQ(shaderChangedCount, 1);

    metallicRoughnessMaterial.SetAlphaCutoff(metallicRoughnessMaterial.GetAlphaCutoff() + 0.1f);
    UpdateScene();
    EXPECT_EQ(shaderChangedCount, 1);
}

/**
 * @tc.name: LoadedMaterial
 * @tc.desc: Tests for Loaded Material. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, LoadedMaterial, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    auto cube = Geometry(scene.FindNode("AnimatedCube"));
    ASSERT_TRUE(cube);
    auto mesh = cube.GetMesh();
    ASSERT_GT(mesh.GetSubMeshCount(), 0);
    auto submesh = mesh.GetSubMeshAt(0);
    ASSERT_TRUE(submesh);
    auto material = MetallicRoughnessMaterial(submesh.GetMaterial());
    EXPECT_TRUE(material);
    EXPECT_EQ(material.GetType(), MaterialType::METALLIC_ROUGHNESS);
    auto image = material.GetBaseColor().GetImage();
    EXPECT_EQ(image.GetSize(), BASE_NS::Math::UVec2(512, 512));
}

/**
 * @tc.name: Image
 * @tc.desc: Tests for Image. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, Image, testing::ext::TestSize.Level1)
{
    auto& scene = GetScene();
    auto image = scene.GetResourceFactory().CreateImage("test://AnimatedCube/AnimatedCube_BaseColor.png");
    ASSERT_TRUE(image);
    EXPECT_EQ(image.GetSize(), BASE_NS::Math::UVec2(512, 512));
}

/**
 * @tc.name: MorphTargets
 * @tc.desc: Tests for Morph Targets. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, MorphTargets, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test://models/morph.gltf"));
    ASSERT_TRUE(scene);
    auto node = Geometry(scene.FindNode("Morph"));
    auto morpher = node.GetMorpher();
    ASSERT_TRUE(morpher);
    auto names = morpher.GetMorphNames();
    auto weights = morpher.GetMorphWeights();
    EXPECT_EQ(names.size(), 2);
    EXPECT_EQ(weights.size(), 2);
    EXPECT_THAT(names, ::testing::ElementsAre("Target1", "Target2"));
    EXPECT_THAT(weights, ::testing::ElementsAre(1.f, .5f));
    morpher.SetMorphWeightAt(1, 1.f);
    weights = morpher.GetMorphWeights();
    EXPECT_THAT(weights, ::testing::ElementsAre(1.f, 1.f));

    // Check generic component access
    auto component = scene.GetComponent(node, "MorphComponent");
    ASSERT_TRUE(component);
    EXPECT_EQ(component, morpher);
    auto cnames = GetValue(component.GetArrayProperty<BASE_NS::string>("MorphNames"));
    auto cweights = GetValue(component.GetArrayProperty<float>("MorphWeights"));
    EXPECT_THAT(cnames, ::testing::ElementsAre("Target1", "Target2"));
    EXPECT_THAT(cweights, ::testing::ElementsAre(1.f, 1.f));
}

/**
 * @tc.name: Animation
 * @tc.desc: Tests for Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, Animation, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    ASSERT_TRUE(scene);
    auto cube = scene.FindNode("AnimatedCube");
    auto animation = scene.FindAnimation("animation_AnimatedCube");
    ASSERT_TRUE(cube);
    ASSERT_TRUE(animation);
    EXPECT_FALSE(animation.GetRunning());
    // Record initial value of the animation's target property
    auto value = cube.GetRotation();
    auto delay = META_NS::TimeSpan::Milliseconds(15);
    UpdateScene(delay);
    EXPECT_EQ(cube.GetRotation(), value);
    // Start animation
    animation.Start();
    EXPECT_TRUE(animation.GetRunning());
    // After start, animation is at initial value
    EXPECT_EQ(cube.GetRotation(), value);
    UpdateScene(delay);
    auto current = cube.GetRotation();
    EXPECT_NE(current, value);
    // Second frame, animation should proceed
    UpdateScene(delay);
    EXPECT_NE(cube.GetRotation(), current);

    animation.Finish();
    EXPECT_EQ(animation.GetProgress(), 1.f);
    UpdateScene(delay);
    EXPECT_EQ(animation.GetProgress(), 1.f);
}

/**
 * @tc.name: ExplicitAnimation
 * @tc.desc: Tests for Explicit Animation. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, ExplicitAnimation, testing::ext::TestSize.Level1)
{
    auto scene = GetScene();
    ASSERT_TRUE(scene);

    auto node = scene.GetResourceFactory().CreateNode("//node");
    EXPECT_EQ(node.GetPosition(), BASE_NS::Math::Vec3(0, 0, 0));

    auto delay = META_NS::TimeSpan::Milliseconds(1);
    auto property = node.Position();
    auto value = node.GetPosition();

    META_NS::KeyframeAnimation<BASE_NS::Math::Vec3> animation(META_NS::CreateNew);

    animation.SetFrom(value).SetTo({ 1, 1, 1 }).SetProperty(property).SetDuration(META_NS::TimeSpan::Seconds(1));
    EXPECT_TRUE(animation.GetValid());
    animation.Start();
    EXPECT_TRUE(animation.GetRunning());

    // First frame after start, animation is at initial value
    UpdateScene(delay);
    EXPECT_EQ(META_NS::GetValue(property), value);
    // Second frame, animation should proceed
    UpdateScene(delay);
    auto current = META_NS::GetValue(property);
    EXPECT_NE(current, value);

    // Check that value has been updated to ECS
    EcsObject ecs(node);
    EcsScene ecss(scene);
    ASSERT_TRUE(ecs);
    ASSERT_TRUE(ecss);
    BASE_NS::Math::Vec3 ecsValue;
    auto trcm = static_cast<CORE3D_NS::ITransformComponentManager*>(
        ecss.GetEcs()->GetComponentManager(CORE3D_NS::ITransformComponentManager::UID));
    if (const auto data = trcm->Read(ecs.GetEntity())) {
        ecsValue = data->position;
    }
    EXPECT_EQ(ecsValue, current);
}

/**
 * @tc.name: LoadScene
 * @tc.desc: Tests for Load Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, LoadScene, testing::ext::TestSize.Level1)
{
    {
        auto scene = Scene(LoadScene("test://scene_resource/test1/test.scene"));
        ASSERT_TRUE(scene);
        auto cube = scene.FindNode("AnimatedCube");
        ASSERT_TRUE(cube);
    }
    {
        auto scene = Scene(LoadScene("test://scene_resource/test2/test.scene"));
        ASSERT_TRUE(scene);
        auto cube = scene.FindNode("AnimatedCube");
        ASSERT_TRUE(cube);
    }
    {
        auto scene = Scene(LoadScene("test://scene_resource/test1/test.scene"));
        ASSERT_TRUE(scene);
        auto cube = scene.FindNode("AnimatedCube");
        ASSERT_TRUE(cube);
    }
}

/**
 * @tc.name: StaticLoadScene
 * @tc.desc: Tests for Static Load Scene. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, StaticLoadScene, testing::ext::TestSize.Level1)
{
    {
        auto scene = Scene::Load("test://scene_resource/test1/test.scene");
        ASSERT_TRUE(scene);
        auto cube = scene.FindNode("AnimatedCube");
        ASSERT_TRUE(cube);
    }
    {
        auto scene = Scene::Load("test://scene_resource/test2/test.scene");
        ASSERT_TRUE(scene);
        auto cube = scene.FindNode("AnimatedCube");
        ASSERT_TRUE(cube);
    }
    {
        auto scene = Scene(Scene::Load<META_NS::Async>("test://scene_resource/test1/test.scene").GetResult());
        ASSERT_TRUE(scene);
        auto cube = scene.FindNode("AnimatedCube");
        ASSERT_TRUE(cube);
    }
}

/**
 * @tc.name: Input
 * @tc.desc: Tests for Input. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_SceneApiTest, Input, testing::ext::TestSize.Level1)
{
    auto scene = Scene(LoadScene("test://AnimatedCube/AnimatedCube.gltf"));
    ASSERT_TRUE(scene);

    auto camera = scene.GetResourceFactory().CreateCameraNode("//camera");
    ASSERT_TRUE(camera);
    auto& reg = META_NS::GetObjectRegistry();

    auto attach = META_NS::AttachmentContainer(camera);
    auto handler1 = reg.Create<IApiBehavior>(::ClassId::ApiBehavior);
    auto handler2 = reg.Create<IApiBehavior>(::ClassId::ApiBehavior);

    ASSERT_TRUE(attach.Attach(handler1));
    ASSERT_TRUE(attach.Attach(handler2));

    camera.SendPointerDown(0, { 0.5, 0.5 });
    camera.SendPointerUp(0, { 0.5, 0.5 });

    EXPECT_EQ(handler1->GetInputEventDownCount(), 1);
    EXPECT_EQ(handler2->GetInputEventDownCount(), 1);
    EXPECT_EQ(handler1->GetInputEventUpCount(), 1);
    EXPECT_EQ(handler2->GetInputEventUpCount(), 1);

    handler1->SetPropagateEvent(false);
    camera.SendPointerDown(0, { 0.5, 0.5 });

    EXPECT_EQ(handler1->GetInputEventDownCount(), 2);
    EXPECT_EQ(handler2->GetInputEventDownCount(), 1);
    EXPECT_EQ(handler1->GetInputEventUpCount(), 1);
    EXPECT_EQ(handler2->GetInputEventUpCount(), 1);

    handler1->SetPropagateEvent(true);
    camera.SendPointerUp(0, { 0.5, 0.5 });

    EXPECT_EQ(handler1->GetInputEventDownCount(), 2);
    EXPECT_EQ(handler2->GetInputEventDownCount(), 1);
    EXPECT_EQ(handler1->GetInputEventUpCount(), 2);
    EXPECT_EQ(handler2->GetInputEventUpCount(), 2);
}

} // namespace UTest
SCENE_END_NAMESPACE()
