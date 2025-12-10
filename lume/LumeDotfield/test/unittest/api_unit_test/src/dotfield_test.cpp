/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/intf_graphics_context.h>
#include <3d/util/intf_scene_util.h>
#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/plugin/intf_class_register.h>
#include <core/property/intf_property_api.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_device.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>

#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include <render/gles/intf_device_gles.h>
#endif
#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/vulkan.h>

#include <render/vulkan/intf_device_vk.h>
#endif

#include <dotfield/ecs/components/dotfield_component.h>
#include <dotfield/ecs/systems/dotfield_system.h>
#include <dotfield/render/intf_render_data_store_default_dotfield.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

namespace Dotfield {
namespace UTest {

class API_DotfieldNodeTest : public ::testing::Test {
    void SetUp() override {}

    void TearDown() override
    {
        // engine, renderContext, graphicsContext and ecs are destroyed by the global test env.
        m_imageHandle = {};
        m_bufferHandle = {};

        GetTestContext()->renderContext->GetRenderer().RenderFrame({});
    }

protected:
    void RenderDotfieldFrame(const RENDER_NS::DeviceBackendType backend)
    {
        // Recreate context if testContext is using a different backend
        TestContext* testContext = nullptr;
        if (GetTestContext()->renderContext->GetDevice().GetBackendType() != backend) {
            testContext = RecreateTestContext(backend);
        } else {
            testContext = GetTestContext();
        }
        auto engine = testContext->engine;
        auto renderContext = testContext->renderContext;
        auto graphicsContext = testContext->graphicsContext;
        auto ecsContext = testContext->ecs;
        const CORE_NS::IEcs& ecsRef = *(ecsContext.get());

        Dotfield::IDotfieldSystem* dotfieldSystem = CORE_NS::GetSystem<Dotfield::IDotfieldSystem>(ecsRef);
        EXPECT_EQ(dotfieldSystem->GetName(), "DotfieldSystem");
        EXPECT_EQ(dotfieldSystem->GetUid(), Dotfield::IDotfieldSystem::UID);
        CORE_NS::IPropertyHandle* properties = dotfieldSystem->GetProperties();
        EXPECT_TRUE(properties);
        EXPECT_TRUE(properties->Owner());
        EXPECT_EQ(properties->Owner()->PropertyCount(), 1);
        BASE_NS::array_view<const CORE_NS::Property> metadata = properties->Owner()->MetaData();
        EXPECT_EQ(metadata.size(), 1);
        EXPECT_EQ(metadata[0].name, "speed");

        CORE3D_NS::INodeSystem* nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecsRef);
        ASSERT_TRUE(nodeSystem);

        auto renderConfigMgr = CORE_NS::GetManager<CORE3D_NS::IRenderConfigurationComponentManager>(ecsRef);
        auto cameraManager = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(ecsRef);

        CORE3D_NS::ISceneNode* rootNode = nodeSystem->CreateNode();
        ASSERT_TRUE(rootNode);

        CORE_NS::Entity rootEntity = rootNode->GetEntity();

        renderConfigMgr->Create(rootEntity);

        CORE3D_NS::ISceneNode* dotfieldNode;

        // create a camera
        CORE_NS::Entity cameraEntity;
        {
            const auto& sceneUtil = graphicsContext->GetSceneUtil();

            CORE3D_NS::INameComponentManager* nameManager =
                CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(ecsRef);
            ASSERT_TRUE(nameManager);

            cameraEntity = sceneUtil.CreateCamera(
                *ecsContext, BASE_NS::Math::Vec3(0, 0, 0), BASE_NS::Math::Quat(0, 0, 0, 1), 0.1f, 100.0f, 60.0f);
            nameManager->Set(cameraEntity, { "MainCamera" });

            auto node = nodeSystem->GetNode(cameraEntity);
            node->SetParent(*rootNode);

            float yfov = 0.0f;
            if (auto cameraHandle = cameraManager->Write(cameraEntity); cameraHandle) {
                CORE3D_NS::CameraComponent& cameraComponent = *cameraHandle;

                cameraComponent.sceneFlags |= CORE3D_NS::CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT;
                cameraComponent.pipelineFlags |= CORE3D_NS::CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT |
                                                 CORE3D_NS::CameraComponent::PipelineFlagBits::DEPTH_OUTPUT_BIT;
                cameraComponent.renderingPipeline = CORE3D_NS::CameraComponent::RenderingPipeline::LIGHT_FORWARD;
                cameraComponent.clearColorValue = { 0.0f, 0.0f, 0.0f, 0.0f };
                cameraComponent.layerMask = CORE3D_NS::LayerConstants::ALL_LAYER_MASK;

                yfov = cameraComponent.yFov;
            }

            sceneUtil.UpdateCameraViewport(*ecsContext, cameraEntity, { 512, 512 }, true, yfov, 1.0f);
        }

        // create dotfield
        {
            dotfieldNode = nodeSystem->CreateNode();
            ASSERT_TRUE(dotfieldNode);

            dotfieldNode->SetParent(*rootNode);

            auto entity = dotfieldNode->GetEntity();

            {
                CORE_NS::IComponentManager* gsComponentManager =
                    ecsContext->GetComponentManager(Dotfield::IDotfieldComponentManager::UID);
                CORE_ASSERT(gsComponentManager);

                gsComponentManager->Create(entity);
                CORE_NS::ScopedHandle<Dotfield::DotfieldComponent> data(gsComponentManager->GetData(entity));
                data->size = { 64.f, 64.f };
                data->pointScale = 60.f;
                data->touchPosition = { 0.f, 0.f };
                data->touchDirection = { 0.f, 0.f, 0.f };
                data->touchRadius = 0.f;
                data->color0 = 0xffffffff;
                data->color1 = 0xffff0000;
                data->color2 = 0xff00ff00;
                data->color3 = 0xff0000ff;
            }
        }

        auto* ecsRawPtr = ecsContext.get();
        const bool needRender = engine->TickFrame(BASE_NS::array_view(&ecsRawPtr, 1));
        ASSERT_TRUE(needRender);

        auto dataStore = static_cast<Dotfield::IRenderDataStoreDefaultDotfield*>(
            renderContext->GetRenderDataStoreManager().GetRenderDataStore("RenderDataStoreDefaultDotfield").get());
        ASSERT_TRUE(dataStore);
        {
            const BASE_NS::array_view<Dotfield::RenderDataDefaultDotfield::DotfieldPrimitive> primitives =
                dataStore->GetDotfieldPrimitives();
            ASSERT_EQ(primitives.size(), 1);
            const Dotfield::RenderDataDefaultDotfield::BufferData& data = dataStore->GetBufferData();
            ASSERT_EQ(data.buffers.size(), 1);
        }

        // image and buffer descriptors
        RENDER_NS::GpuImageDesc imageDesc;
        imageDesc.width = 512;
        imageDesc.height = 512;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = RENDER_NS::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM;
        imageDesc.imageTiling = RENDER_NS::CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = RENDER_NS::CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = RENDER_NS::CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = RENDER_NS::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags =
            RENDER_NS::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | RENDER_NS::CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
        m_imageHandle = renderContext->GetDevice().GetGpuResourceManager().Create("TestOutputImage", imageDesc);

        RENDER_NS::GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = 512 * 512 * 4;
        bufferDesc.engineCreationFlags = RENDER_NS::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = RENDER_NS::CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags =
            RENDER_NS::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | RENDER_NS::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        m_bufferHandle = renderContext->GetDevice().GetGpuResourceManager().Create("TestOutputBuffer", bufferDesc);

        {
            RENDER_NS::IRenderFrameUtil::BackBufferConfiguration bbc;
            bbc.backBufferType =
                RENDER_NS::IRenderFrameUtil::BackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY;
            bbc.present = false;
            bbc.backBufferHandle = m_imageHandle;
            bbc.gpuBufferHandle = m_bufferHandle;
            renderContext->GetRenderUtil().GetRenderFrameUtil().SetBackBufferConfiguration(bbc);
        }
        m_byteArray = BASE_NS::make_unique<BASE_NS::ByteArray>(bufferDesc.byteSize);

        ASSERT_TRUE(m_byteArray);

        // render a frame
        {
            RENDER_NS::IRenderer& renderer = renderContext->GetRenderer();
            const auto ecsRngs = graphicsContext->GetRenderNodeGraphs(*ecsContext);
            BASE_NS::vector<RENDER_NS::RenderHandleReference> rngs;
            rngs.append(ecsRngs.begin(), ecsRngs.end());
            renderer.RenderFrame(rngs);
        }

        nodeSystem->DestroyNode(*dotfieldNode);
        engine->TickFrame(BASE_NS::array_view(&ecsRawPtr, 1));
        {
            const BASE_NS::array_view<Dotfield::RenderDataDefaultDotfield::DotfieldPrimitive> primitives =
                dataStore->GetDotfieldPrimitives();
            ASSERT_EQ(primitives.size(), 0);
            const Dotfield::RenderDataDefaultDotfield::BufferData& data = dataStore->GetBufferData();
            ASSERT_EQ(data.buffers.size(), 0);
        }
    }

private:
    RENDER_NS::DeviceBackendType m_backend;
    RENDER_NS::RenderHandleReference m_imageHandle;
    RENDER_NS::RenderHandleReference m_bufferHandle;
    BASE_NS::unique_ptr<BASE_NS::ByteArray> m_byteArray;
};

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: DotfieldNodeRenderVulkan
 * @tc.desc: Tests Dotfield node with Vulkan backend.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_DotfieldNodeTest, DotfieldNodeRenderVulkan, testing::ext::TestSize.Level1)
{
    RenderDotfieldFrame(RENDER_NS::DeviceBackendType::VULKAN);
}
#endif

#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
/**
 * @tc.name: DotfieldNodeRenderGL
 * @tc.desc: Tests Dotfield node with GL backend.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_DotfieldNodeTest, DotfieldNodeRenderGL, testing::ext::TestSize.Level1)
{
#if RENDER_HAS_GLES_BACKEND
    RenderDotfieldFrame(RENDER_NS::DeviceBackendType::OPENGLES);
#else
    RenderDotfieldFrame(RENDER_NS::DeviceBackendType::OPENGL);
#endif
}
#endif

} // namespace UTest
} // namespace Dotfield
