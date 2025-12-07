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
#ifndef SCENE_TEST_SCENE_TEST_H
#define SCENE_TEST_SCENE_TEST_H

#include <chrono>
#include <cinttypes>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_mesh.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/resource/intf_render_resource_manager.h>
#include <thread>

#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <core/property/scoped_handle.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_renderer.h>

#include <meta/api/engine/util.h>
#include <meta/api/object_name.h>
#include <meta/interface/intf_manual_clock.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>

#include "test_framework.h"

META_TYPE(CORE3D_NS::MeshComponent::Submesh)

SCENE_BEGIN_NAMESPACE()
namespace UTest {

template<typename Type>
Type ReadValueFromEngine(const META_NS::EnginePropertyParams& params)
{
    Type res {};
    if (CORE_NS::ScopedHandle<const Type> guard { params.handle.Handle() }) {
        res = *(const Type*)((uintptr_t) & *guard + params.Offset());
    }
    return res;
}

class ScenePluginTest : public ::testing::Test {
public:
    IScene::Ptr scene;
    IRenderContext::Ptr context;
    META_NS::IMetadata::Ptr params;
    CORE_NS::IResourceManager::Ptr resources;
    META_NS::IManualClock::Ptr clock;

    IScene::Ptr CreateEmptyScene()
    {
        if (auto m = GetSceneManager()) {
            scene = m->CreateScene().GetResult();
        }
        return scene;
    }
    IScene::Ptr LoadScene(BASE_NS::string_view url)
    {
        if (auto m = GetSceneManager()) {
            scene = m->CreateScene(url).GetResult();
        }
        return scene;
    }
    META_NS::IMetadata::Ptr GetSceneParams() const
    {
        return params;
    }
    META_NS::IClock::Ptr GetClock() const
    {
        return clock;
    }
    ISceneManager::Ptr GetSceneManager() const
    {
        return ::SCENE_NS::GetSceneManager(context);
    }
    void UpdateSceneAndRenderFrame(
        const IScene::Ptr& sc = nullptr, const META_NS::TimeSpan& delay = META_NS::TimeSpan::Infinite())
    {
        auto s = sc ? sc : scene;
        ASSERT_TRUE(s);
        auto is = sc->GetInternalScene();
        ASSERT_TRUE(is);
        auto rc = is->GetContext()->GetRenderer();
        ASSERT_TRUE(rc);
        UpdateScene(s, delay);
        rc->GetRenderer().RenderFrame({});
    }
    void UpdateScene(const META_NS::TimeSpan& delay = META_NS::TimeSpan::Infinite())
    {
        UpdateScene(scene, delay);
    }
    void UpdateScene(const IScene::Ptr& sc, const META_NS::TimeSpan& delay = META_NS::TimeSpan::Infinite())
    {
        auto d = delay;
        if (d == META_NS::TimeSpan::Infinite()) {
            d = META_NS::TimeSpan::Milliseconds(1);
        }
        clock->SetTime(clock->GetTime() + d);
        META_NS::GetAnimationController()->Step(clock);
        if (sc) {
            sc->GetInternalScene()
                ->AddTask([&] {
                    IInternalScene::UpdateInfo info;
                    info.syncProperties = true;
                    info.clock = clock;
                    sc->GetInternalScene()->Update(info);
                    context->GetRenderer()->GetRenderer().RenderDeferredFrame();
                })
                .Wait();
            WaitForUserQueue();
        }
    }

    template<typename Type>
    void CheckNativePropertyValue(META_NS::IProperty::ConstPtr p, const Type& v)
    {
        CheckNativePropertyValue<Type>(p, v, [](const Type& a, const Type& b) { return a == b; });
    }

    template<typename Type>
    void CheckNativePropertyValue(
        META_NS::IProperty::ConstPtr p, const Type& v, std::function<bool(const Type&, const Type&)> isEqual)
    {
        if (auto ev = interface_cast<META_NS::IEngineValueInternal>(META_NS::GetEngineValueFromProperty(p))) {
            auto pv = scene->GetInternalScene()
                          ->AddTask([&] { return isEqual(v, ReadValueFromEngine<Type>(ev->GetPropertyParams())); })
                          .GetResult();
            EXPECT_TRUE(pv);
        }
    }

    IMaterial::Ptr CreateMaterial(META_NS::ObjectId materialType = ClassId::Material) const
    {
        return scene->CreateObject<IMaterial>(materialType).GetResult();
    }

    CORE3D_NS::MaterialComponent::Shader CreateMaterialComponentShader() const
    {
        auto man = scene->CreateObject<IRenderResourceManager>(ClassId::RenderResourceManager).GetResult();
        if (!man) {
            return {};
        }
        auto shader = man->LoadShader("test://shaders/test.shader").GetResult();
        CORE_NS::EntityReference shaderRef;
        CORE_NS::EntityReference graphicsStateRef;
        if (auto i = interface_cast<IRenderResource>(shader)) {
            shaderRef = scene->GetInternalScene()->GetEcsContext().GetRenderHandleEntity(i->GetRenderHandle());
        }
        if (auto i = interface_cast<IGraphicsState>(shader)) {
            graphicsStateRef = scene->GetInternalScene()->GetEcsContext().GetRenderHandleEntity(i->GetGraphicsState());
        }

        EXPECT_TRUE(shaderRef && graphicsStateRef);
        return CORE3D_NS::MaterialComponent::Shader { shaderRef, graphicsStateRef };
    }

    IImage::Ptr CreateTestBitmap()
    {
        if (auto bmap = scene->CreateObject<IImage>(ClassId::Image).GetResult()) {
            RENDER_NS::GpuImageDesc desc;
            desc.width = 128;
            desc.height = 128;
            desc.depth = 1;
            desc.engineCreationFlags = RENDER_NS::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
            desc.format = BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT;
            desc.imageType = RENDER_NS::ImageType::CORE_IMAGE_TYPE_2D;
            desc.memoryPropertyFlags = RENDER_NS::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            desc.usageFlags = RENDER_NS::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            auto handle =
                scene->GetInternalScene()->GetRenderContext().GetDevice().GetGpuResourceManager().Create(desc);
            if (auto res = interface_cast<IRenderResource>(bmap)) {
                EXPECT_TRUE(res->SetRenderHandle(handle));
            }
            return bmap;
        }
        return nullptr;
    }

    void WaitForUserQueue()
    {
        context->AddTask([] {}, context->GetApplicationQueue()).Wait();
    }

    void AddSubMesh(IMesh::Ptr mesh)
    {
        if (auto i = interface_pointer_cast<IMeshAccess>(mesh)) {
            mesh = i->GetMesh().GetResult();
        }
        auto acc = interface_cast<IEcsObjectAccess>(mesh);
        ASSERT_TRUE(acc);
        auto ecsObj = acc->GetEcsObject();
        META_NS::ArrayProperty<CORE3D_NS::MeshComponent::Submesh> arr(
            ecsObj->CreateProperty("MeshComponent.submeshes").GetResult());
        ASSERT_TRUE(arr);
        arr->AddValue({});
        UpdateScene();
        // wait for running notifications on changed engine properties
        WaitForUserQueue();
    }

    size_t GetEntityCount() const
    {
        size_t size {};
        auto& m = scene->GetInternalScene()->GetEcsContext().GetNativeEcs()->GetEntityManager();

        {
            auto it = m.Begin(CORE_NS::IEntityManager::IteratorType::ALIVE);
            auto end = m.End(CORE_NS::IEntityManager::IteratorType::ALIVE);
            for (; it && !it->Compare(end); it->Next(), ++size) {
            }
        }
        {
            auto it = m.Begin(CORE_NS::IEntityManager::IteratorType::DEACTIVATED);
            auto end = m.End(CORE_NS::IEntityManager::IteratorType::DEACTIVATED);
            for (; it && !it->Compare(end); it->Next(), ++size) {
            }
        }
        return size;
    }

    void LogAliveEntities(IScene::Ptr scene) const
    {
        size_t size {};
        auto& m = scene->GetInternalScene()->GetEcsContext().GetNativeEcs()->GetEntityManager();

        {
            auto it = m.Begin(CORE_NS::IEntityManager::IteratorType::ALIVE);
            auto end = m.End(CORE_NS::IEntityManager::IteratorType::ALIVE);
            for (; it && !it->Compare(end); it->Next(), ++size) {
                CORE_LOG_I("Entity %zu %" PRIx64, size, it->Get().id);
            }
        }
    }

protected:
    void SetUp() override
    {
        auto appc = GetDefaultApplicationContext();
        ASSERT_TRUE(appc);

        clock = META_NS::GetObjectRegistry().Create<META_NS::IManualClock>(META_NS::ClassId::ManualClock);
        clock->SetTime(META_NS::TimeSpan::Zero());

        context = appc->GetRenderContext();
        ASSERT_TRUE(context);
        params = CreateRenderContextArg(context);

        resources = context->GetResources();
        // remove all resources other tests might have added
        resources->RemoveAllResources();

        // "render" one frame. this is to initialize all the default resources etc.
        // if we never render a single frame, we will get "false positive" leaks of gpu resources.
        context->GetRenderer()->GetRenderer().RenderFrame({});
    }

    void TearDown() override
    {
        context.reset();
        params.reset();
        scene.reset();
        resources.reset();
    }
};

inline void CheckNativeEcsPath(INode::Ptr node, const BASE_NS::string& path)
{
    auto nodeEcs = interface_pointer_cast<IEcsObjectAccess>(node);
    ASSERT_TRUE(nodeEcs);
    auto ecsObj = nodeEcs->GetEcsObject();
    ASSERT_TRUE(ecsObj);
    EXPECT_EQ(node->GetScene()->GetInternalScene()->GetEcsContext().GetPath(ecsObj->GetEntity()), path);
}

inline void PrintHierarchy(INode::Ptr n, size_t depth = 0)
{
    BASE_NS::string tab(depth, '-');
    printf("%s%s\n", tab.c_str(), n->GetName().c_str());
    META_NS::Internal::ConstIterate(
        interface_cast<META_NS::IIterable>(n),
        [&](META_NS::IObject::Ptr nodeObj) {
            if (auto node = interface_pointer_cast<INode>(nodeObj)) {
                PrintHierarchy(node, depth + 1);
            }
            return true;
        },
        META_NS::IterateStrategy { META_NS::TraversalType::NO_HIERARCHY });
}

} // namespace UTest
SCENE_END_NAMESPACE()

#endif
