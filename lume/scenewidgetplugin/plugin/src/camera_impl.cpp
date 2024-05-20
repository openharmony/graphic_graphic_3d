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
#include <scene_plugin/api/camera_uid.h>
#include <scene_plugin/api/postprocess_uid.h>

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "intf_postprocess_private.h"
#include "node_impl.h"
#include "task_utils.h"

using SCENE_NS::MakeTask;

namespace {
class CameraImpl
    : public META_NS::ConcreteBaseMetaObjectFwd<CameraImpl, NodeImpl, SCENE_NS::ClassId::Camera, SCENE_NS::ICamera> {
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, FoV, 60.f * BASE_NS::Math::DEG2RAD)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, AspectRatio, -1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, NearPlane, 0.3f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, FarPlane, 1000.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, XMagnification, 0.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, YMagnification, 0.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, XOffset, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, YOffset, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ICamera, uint8_t, Projection, SCENE_NS::ICamera::SCENE_CAM_PROJECTION_PERSPECTIVE)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ICamera, uint8_t, Culling, SCENE_NS::ICamera::SCENE_CAM_CULLING_VIEW_FRUSTUM)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ICamera, uint8_t, RenderingPipeline, SCENE_NS::ICamera::SCENE_CAM_PIPELINE_FORWARD)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, uint32_t, SceneFlags, 0)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, uint32_t, PipelineFlags, (1 << 3))
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ICamera, BASE_NS::Math::Vec4, Viewport, BASE_NS::Math::Vec4(0.f, 0.f, 1.f, 1.f))
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ICamera, BASE_NS::Math::Vec4, Scissor, BASE_NS::Math::Vec4(0.f, 0.f, 1.f, 1.f))
    META_IMPLEMENT_INTERFACE_PROPERTY(
        SCENE_NS::ICamera, BASE_NS::Math::UVec2, RenderTargetSize, BASE_NS::Math::UVec2(0, 0))
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, SCENE_NS::Color, ClearColor, SCENE_NS::Colors::TRANSPARENT)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, float, ClearDepth, 1.f)
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, BASE_NS::string, RenderNodeGraphFile, {})
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::ICamera, SCENE_NS::IPostProcess::Ptr, PostProcess, {})

    static constexpr BASE_NS::string_view CAMERA_COMPONENT_NAME = "CameraComponent";
    static constexpr size_t CAMERA_COMPONENT_NAME_LEN = CAMERA_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view CAMERA_FOV = "CameraComponent.yFov";
    static constexpr BASE_NS::string_view CAMERA_ASPECT = "CameraComponent.aspect";
    static constexpr BASE_NS::string_view CAMERA_ZNEAR = "CameraComponent.zNear";
    static constexpr BASE_NS::string_view CAMERA_ZFAR = "CameraComponent.zFar";
    static constexpr BASE_NS::string_view CAMERA_XMAG = "CameraComponent.xMag";
    static constexpr BASE_NS::string_view CAMERA_YMAG = "CameraComponent.yMag";
    static constexpr BASE_NS::string_view CAMERA_XOFFSET = "CameraComponent.xOffset";
    static constexpr BASE_NS::string_view CAMERA_YOFFSET = "CameraComponent.yOffset";
    static constexpr BASE_NS::string_view CAMERA_PROJECTION = "CameraComponent.projection";
    static constexpr BASE_NS::string_view CAMERA_CULLING = "CameraComponent.culling";
    static constexpr BASE_NS::string_view CAMERA_RENDERINGPIPELINE = "CameraComponent.renderingPipeline";
    static constexpr BASE_NS::string_view CAMERA_SCENEFLAGS = "CameraComponent.sceneFlags";
    static constexpr BASE_NS::string_view CAMERA_PIPELINEFLAGS = "CameraComponent.pipelineFlags";
    static constexpr BASE_NS::string_view CAMERA_SCISSOR = "CameraComponent.scissor";
    static constexpr BASE_NS::string_view CAMERA_VIEWPORT = "CameraComponent.viewport";
    static constexpr BASE_NS::string_view CAMERA_CLEARCOLORVALUE = "CameraComponent.clearColorValue";
    static constexpr BASE_NS::string_view CAMERA_CLEARDEPTHVALUE = "CameraComponent.clearDepthValue";
    static constexpr BASE_NS::string_view CAMERA_LAYERMASK = "CameraComponent.layerMask";
    static constexpr BASE_NS::string_view CAMERA_RENDERRESOLUTION = "CameraComponent.renderResolution";
    static constexpr BASE_NS::string_view CAMERA_POSTPROCESS = "CameraComponent.postProcess";

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = false;
        if (ret = NodeImpl::Build(data); ret) {
            PropertyNameMask()[CAMERA_COMPONENT_NAME] = { CAMERA_FOV.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_ASPECT.substr(CAMERA_COMPONENT_NAME_LEN), CAMERA_ZNEAR.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_ZFAR.substr(CAMERA_COMPONENT_NAME_LEN), CAMERA_XMAG.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_YMAG.substr(CAMERA_COMPONENT_NAME_LEN), CAMERA_XOFFSET.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_YOFFSET.substr(CAMERA_COMPONENT_NAME_LEN), CAMERA_PROJECTION.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_CULLING.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_RENDERINGPIPELINE.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_SCENEFLAGS.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_PIPELINEFLAGS.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_SCISSOR.substr(CAMERA_COMPONENT_NAME_LEN), CAMERA_VIEWPORT.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_CLEARCOLORVALUE.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_CLEARDEPTHVALUE.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_LAYERMASK.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_POSTPROCESS.substr(CAMERA_COMPONENT_NAME_LEN),
                CAMERA_RENDERRESOLUTION.substr(CAMERA_COMPONENT_NAME_LEN) };
        }
        return ret;
    }

    bool CompleteInitialization(const BASE_NS::string& path) override
    {
        if (!NodeImpl::CompleteInitialization(path)) {
            return false;
        }

        auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(FoV), meta, CAMERA_FOV);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(AspectRatio), meta, CAMERA_ASPECT);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(NearPlane), meta, CAMERA_ZNEAR);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(FarPlane), meta, CAMERA_ZFAR);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(XMagnification), meta, CAMERA_XMAG);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(YMagnification), meta, CAMERA_YMAG);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(XOffset), meta, CAMERA_XOFFSET);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(YOffset), meta, CAMERA_YOFFSET);
        BindChanges<uint8_t>(propHandler_, META_ACCESS_PROPERTY(Projection), meta, CAMERA_PROJECTION);
        BindChanges<uint8_t>(propHandler_, META_ACCESS_PROPERTY(Culling), meta, CAMERA_CULLING);
        BindChanges<uint8_t>(propHandler_, META_ACCESS_PROPERTY(RenderingPipeline), meta, CAMERA_RENDERINGPIPELINE);
        BindChanges<uint32_t>(propHandler_, META_ACCESS_PROPERTY(SceneFlags), meta, CAMERA_SCENEFLAGS);
        BindChanges<uint32_t>(propHandler_, META_ACCESS_PROPERTY(PipelineFlags), meta, CAMERA_PIPELINEFLAGS);
        BindChanges<BASE_NS::Math::Vec4>(propHandler_, META_ACCESS_PROPERTY(Scissor), meta, CAMERA_SCISSOR);
        BindChanges<BASE_NS::Math::Vec4>(propHandler_, META_ACCESS_PROPERTY(Viewport), meta, CAMERA_VIEWPORT);
        ConvertBindChanges<SCENE_NS::Color, BASE_NS::Math::Vec4>(
            propHandler_, META_ACCESS_PROPERTY(ClearColor), meta, CAMERA_CLEARCOLORVALUE);
        BindChanges<float>(propHandler_, META_ACCESS_PROPERTY(ClearDepth), meta, CAMERA_CLEARDEPTHVALUE);

        // override INode default handling for LayerMask
        BindChanges<uint64_t>(propHandler_, NodeImpl::META_ACCESS_PROPERTY(LayerMask), meta, CAMERA_LAYERMASK);

        if (auto scene = GetScene()) {
            propHandler_.NewHandler(nullptr, nullptr)
                .Subscribe(META_ACCESS_PROPERTY(RenderTargetSize),
                    META_NS::MakeCallback<META_NS::IOnChanged>(
                        [this](const auto& scene) {
                            if (auto ecsObject = EcsObject(); scene && ecsObject) {
                                auto renderSize = META_NS::GetValue(RenderTargetSize());
                                if (renderSize.x != 0 && renderSize.y != 0) {
                                    SceneHolder()->SetRenderSize(renderSize.x, renderSize.y, ecsObject->GetEntity().id);
                                }
                            }
                        },
                        scene));
        } else {
            // Engine side does not automatically resize camera resources even the size changes
            BindChanges<BASE_NS::Math::UVec2>(
                propHandler_, META_ACCESS_PROPERTY(RenderTargetSize), meta, CAMERA_RENDERRESOLUTION);
        }

        if (auto postProcess = meta->GetPropertyByName(CAMERA_POSTPROCESS)) {
            // To quickly test / verify post-process bindings
            // callback to carry out when toolkit side changes
            auto metaCallback = [this](META_NS::IProperty::Ptr postProcessEntity,
                                    SCENE_NS::IPostProcess::Ptr postProcessBridge) {
                auto typed = META_NS::Property<CORE_NS::Entity>(postProcessEntity);
                auto entity = META_NS::GetValue(typed);
                auto sh = SceneHolder();
                auto ecs = sh->GetEcs();
                if (!CORE_NS::EntityUtil::IsValid(entity)) {
                    // engine does not have existing entity, create new one
                    entity = sh->CreatePostProcess();
                    META_NS::SetValue(typed, entity);
                }
                // values from toolkit dominate initialization
                interface_cast<IPostProcessPrivate>(postProcessBridge)->SetEntity(entity, sh, false);
            };

            // callback to carry out if the engine side drives the change
            auto engineCallback = [this](META_NS::IProperty::Ptr postProcessEntity) {
                auto entity = META_NS::GetValue(META_NS::Property<CORE_NS::Entity>(postProcessEntity));
                auto sh = SceneHolder();
                auto ecs = sh->GetEcs();
                if (!CORE_NS::EntityUtil::IsValid(entity)) {
                    META_NS::SetValue(PostProcess(), SCENE_NS::IPostProcess::Ptr {});
                } else {
                    auto ppo = META_NS::GetObjectRegistry().Create<IPostProcessPrivate>(SCENE_NS::ClassId::PostProcess);
                    // values from ecs dominate initialization
                    ppo->SetEntity(entity, sh, true);
                    META_NS::SetValue(PostProcess(), interface_pointer_cast<SCENE_NS::IPostProcess>(ppo));
                }
            };

            if (auto bridge = META_NS::GetValue(PostProcess())) {
                metaCallback(postProcess, bridge);
            } else {
                engineCallback(postProcess);
            }

            // setup subscription for toolkit changes
            PostProcess()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this, metaCallback]() {
                if (auto bridge = META_NS::GetValue(PostProcess())) {
                    if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
                        if (auto postProcess = meta->GetPropertyByName(CAMERA_POSTPROCESS)) {
                            metaCallback(postProcess, bridge);
                        }
                    }
                } else {
                    if (auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_)) {
                        if (auto postProcess = meta->GetPropertyByName(CAMERA_POSTPROCESS)) {
                            if (auto typed = META_NS::Property<CORE_NS::Entity>(postProcess)) {
                                META_NS::SetValue(typed, CORE_NS::Entity());
                            }
                        }
                    }
                }
            }),
                reinterpret_cast<uint64_t>(this));

            // setup subscription to ecs changes
            postProcess->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([this, engineCallback]() {
                if (auto meta = this->GetSelf<META_NS::IMetadata>()) {
                    if (auto postProcess = meta->GetPropertyByName(CAMERA_POSTPROCESS)) {
                        engineCallback(postProcess);
                    }
                }
            }),
                reinterpret_cast<uint64_t>(this));
            ConvertBindChanges<SCENE_NS::IPostProcess::Ptr, CORE_NS::Entity, EntityConverter<SCENE_NS::IPostProcess>>(
                propHandler_, PostProcess(), meta, "CameraComponent.postProcess", ONE_WAY_TO_ECS);
        }

        auto updateCustom = [this]() {
            auto pipeline = RenderingPipeline()->GetValue();
            if (pipeline == SCENE_NS::ICamera::SceneCameraPipeline::SCENE_CAM_PIPELINE_FORWARD) {
                auto ecs0 = interface_cast<SCENE_NS::IEcsObject>(GetSelf());
                auto ecs = ecs0->GetEcs();
                auto ent = ecs0->GetEntity();
                if (auto cameraManager = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(*ecs)) {
                    if (cameraManager->HasComponent(ent)) {
                        auto data = cameraManager->Get(ent);
                        data.colorTargetCustomization.clear();
                        data.colorTargetCustomization.push_back({ BASE_NS::BASE_FORMAT_R16G16B16A16_SFLOAT, {} });
                        cameraManager->Set(ent, data);
                    }
                }
            };
        };
        updateCustom();
        RenderingPipeline()->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>(updateCustom), reinterpret_cast<uint64_t>(this));
        return true;
    }

    void Destroy() override {}

    BASE_NS::vector<SCENE_NS::ICamera::Ptr> multiviewCameras_;

    // ToDo: Should someday listen to flags (or even entity changes from engine in case multiview is set up there)
    void AddMultiviewCamera(ICamera::Ptr camera) override
    {
        if (camera) {
            // add render bit
            auto flags = META_NS::GetValue(camera->PipelineFlags()) | CORE3D_NS::CameraComponent::MULTI_VIEW_ONLY_BIT;
            camera->PipelineFlags()->SetValue(flags);
            // store to array and ecs
            if (auto sh = SceneHolder()) {
                sh->QueueEngineTask(MakeTask(
                                        [source = interface_cast<SCENE_NS::IEcsObject>(camera)->GetEntity(),
                                            target = GetSelf<SCENE_NS::IEcsObject>()->GetEntity()](auto sh) {
                                            sh->SetMultiviewCamera(target, source);
                                            return false;
                                        },
                                        sh),
                    false);
            }
            // hold strong ref (allow adding multiple times)
            multiviewCameras_.push_back(camera);
        }
    }

    void RemoveMultiviewCamera(const ICamera::Ptr& camera) override
    {
        if (!camera) {
            return;
        }
        size_t count = 0;
        // release refs
        for (size_t ii = 0; ii < multiviewCameras_.size();) {
            if (multiviewCameras_[ii] == camera) // assuming shared ptr compares the object it points
            {
                if (++count == 1) {
                    multiviewCameras_.erase(multiviewCameras_.cbegin() + ii);
                } else {
                    break;
                }
            } else {
                ii++;
            }
        }

        if (count == 1) {
            // remove render-bit
            auto flags = META_NS::GetValue(camera->PipelineFlags()) & ~CORE3D_NS::CameraComponent::MULTI_VIEW_ONLY_BIT;
            camera->PipelineFlags()->SetValue(flags);

            // tell ecs
            if (auto sh = SceneHolder()) {
                sh->QueueEngineTask(MakeTask(
                                        [source = interface_cast<SCENE_NS::IEcsObject>(camera)->GetEntity(),
                                            target = GetSelf<SCENE_NS::IEcsObject>()->GetEntity()](auto sh) {
                                            sh->RemoveMultiviewCamera(target, source);
                                            return false;
                                        },
                                        sh),
                    false);
            }
        }
    }
    SCENE_NS::IPickingResult::Ptr ScreenToWorld(BASE_NS::Math::Vec3 screenCoordinate) const override
    {
        auto ret = objectRegistry_->Create<SCENE_NS::IPickingResult>(SCENE_NS::ClassId::PendingVec3Request);
        if (ret && SceneHolder()) {
            SceneHolder()->QueueEngineTask(
                META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                    [screenCoordinate, weakRet = BASE_NS::weak_ptr(ret), weakSh = sceneHolder_,
                        weakSelf = BASE_NS::weak_ptr(ecsObject_)]() {
                        if (auto sh = weakSh.lock()) {
                            if (auto ret = weakRet.lock()) {
                                if (auto self = weakSelf.lock()) {
                                    if (sh->ScreenToWorld(ret, self->GetEntity(), screenCoordinate)) {
                                        sh->QueueApplicationTask(
                                            META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet]() {
                                                if (auto writable = interface_pointer_cast<
                                                        SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(weakRet)) {
                                                    writable->MarkReady();
                                                }
                                                return false;
                                            }),
                                            false);
                                    }
                                }
                            }
                        }
                        return false;
                    }),
                false);
            return ret;
        }
        return SCENE_NS::IPickingResult::Ptr();
    }

    SCENE_NS::IPickingResult::Ptr WorldToScreen(BASE_NS::Math::Vec3 worldCoordinate) const override
    {
        auto ret = objectRegistry_->Create<SCENE_NS::IPickingResult>(SCENE_NS::ClassId::PendingVec3Request);
        if (ret && SceneHolder()) {
            SceneHolder()->QueueEngineTask(
                META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                    [worldCoordinate, weakRet = BASE_NS::weak_ptr(ret), weakSh = sceneHolder_,
                        weakSelf = BASE_NS::weak_ptr(ecsObject_)]() {
                        if (auto sh = weakSh.lock()) {
                            if (auto ret = weakRet.lock()) {
                                if (auto self = weakSelf.lock()) {
                                    if (sh->WorldToScreen(ret, self->GetEntity(), worldCoordinate)) {
                                        sh->QueueApplicationTask(
                                            META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet]() {
                                                if (auto writable = interface_pointer_cast<
                                                        SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(weakRet)) {
                                                    writable->MarkReady();
                                                }
                                                return false;
                                            }),
                                            false);
                                    }
                                }
                            }
                        }
                        return false;
                    }),
                false);
            return ret;
        }
        return SCENE_NS::IPickingResult::Ptr();
    }

    SCENE_NS::IRayCastResult::Ptr RayCastFromCamera(const BASE_NS::Math::Vec2& screenPos) const override
    {
        auto ret =
            META_NS::GetObjectRegistry().Create<SCENE_NS::IRayCastResult>(SCENE_NS::ClassId::PendingDistanceRequest);
        if (ret && SceneHolder()) {
            SceneHolder()->QueueEngineTask(
                META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                    [pos = screenPos, weakRet = BASE_NS::weak_ptr(ret), weakSh = sceneHolder_,
                        weakSelf = BASE_NS::weak_ptr(ecsObject_), weakScene = BASE_NS::weak_ptr(GetScene())]() {
                        if (auto sh = weakSh.lock()) {
                            if (auto ret = weakRet.lock()) {
                                if (auto self = weakSelf.lock()) {
                                    if (sh->RayCastFromCamera(ret, self->GetEntity(), pos)) {
                                        sh->QueueApplicationTask(
                                            META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet, weakScene]() {
                                                if (auto writable = interface_pointer_cast<
                                                        SCENE_NS::IPendingRequestData<SCENE_NS::NodeDistance>>(
                                                        weakRet)) {
                                                    if (auto scene = weakScene.lock()) {
                                                        // resolve proxy nodes
                                                        for (size_t ii = writable->MetaData().size(); ii > 0;) {
                                                            --ii;
                                                            writable->MutableData().at(ii).node =
                                                                scene->GetNode(writable->MetaData().at(ii));
                                                        }
                                                        writable->MarkReady();
                                                    }
                                                }
                                                return false;
                                            }),
                                            false);
                                    }
                                }
                            }
                        }
                        return false;
                    }),
                false);
            return ret;
        }
        return SCENE_NS::IRayCastResult::Ptr();
    }

    SCENE_NS::IRayCastResult::Ptr RayCastFromCamera(
        const BASE_NS::Math::Vec2& screenPos, uint64_t layerMask) const override
    {
        auto ret = objectRegistry_->Create<SCENE_NS::IRayCastResult>(SCENE_NS::ClassId::PendingDistanceRequest);
        if (ret && SceneHolder()) {
            SceneHolder()->QueueEngineTask(
                META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                    [pos = screenPos, weakRet = BASE_NS::weak_ptr(ret), weakSh = sceneHolder_, layerMask,
                        weakSelf = BASE_NS::weak_ptr(ecsObject_), weakScene = BASE_NS::weak_ptr(GetScene())]() {
                        if (auto sh = weakSh.lock()) {
                            if (auto ret = weakRet.lock()) {
                                if (auto self = weakSelf.lock()) {
                                    if (sh->RayCastFromCamera(ret, self->GetEntity(), pos, layerMask)) {
                                        sh->QueueApplicationTask(
                                            META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet, weakScene]() {
                                                if (auto writable = interface_pointer_cast<
                                                        SCENE_NS::IPendingRequestData<SCENE_NS::NodeDistance>>(
                                                        weakRet)) {
                                                    if (auto scene = weakScene.lock()) {
                                                        // resolve proxy nodes
                                                        for (size_t ii = writable->MetaData().size(); ii > 0;) {
                                                            --ii;
                                                            writable->MutableData().at(ii).node =
                                                                scene->GetNode(writable->MetaData().at(ii));
                                                        }
                                                        writable->MarkReady();
                                                    }
                                                }
                                                return false;
                                            }),
                                            false);
                                    }
                                }
                            }
                        }
                        return false;
                    }),
                false);
            return ret;
        }
        return SCENE_NS::IRayCastResult::Ptr();
    }

    SCENE_NS::IPickingResult::Ptr RayFromCamera(const BASE_NS::Math::Vec2& screenPos) const override
    {
        auto ret = objectRegistry_->Create<SCENE_NS::IPickingResult>(SCENE_NS::ClassId::PendingVec3Request);
        if (ret && SceneHolder()) {
            SceneHolder()->QueueEngineTask(
                META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                    [pos = screenPos, weakRet = BASE_NS::weak_ptr(ret), weakSh = sceneHolder_,
                        weakSelf = BASE_NS::weak_ptr(ecsObject_), weakScene = BASE_NS::weak_ptr(GetScene())]() {
                        if (auto sh = weakSh.lock()) {
                            if (auto ret = weakRet.lock()) {
                                if (auto self = weakSelf.lock()) {
                                    if (sh->RayFromCamera(ret, self->GetEntity(), pos)) {
                                        sh->QueueApplicationTask(
                                            META_NS::MakeCallback<META_NS::ITaskQueueTask>([weakRet, weakScene]() {
                                                if (auto writable = interface_pointer_cast<
                                                        SCENE_NS::IPendingRequestData<SCENE_NS::NodeDistance>>(
                                                        weakRet)) {
                                                    if (auto scene = weakScene.lock()) {
                                                        // resolve proxy nodes
                                                        for (size_t ii = writable->MetaData().size(); ii > 0;) {
                                                            --ii;
                                                            writable->MutableData().at(ii).node =
                                                                scene->GetNode(writable->MetaData().at(ii));
                                                        }
                                                        writable->MarkReady();
                                                    }
                                                }
                                                return false;
                                            }),
                                            false);
                                    }
                                }
                            }
                        }
                        return false;
                    }),
                false);
            return ret;
        }
        return SCENE_NS::IPickingResult::Ptr();
    }

    void SetDefaultRenderTargetSize(uint64_t width, uint64_t height) override
    {
        RenderTargetSize()->SetDefaultValue(BASE_NS::Math::UVec2(width, height));
    }
};

} // namespace

SCENE_BEGIN_NAMESPACE()

void RegisterCameraImpl()
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.RegisterObjectType<CameraImpl>();
}

void UnregisterCameraImpl()
{
    auto& registry = META_NS::GetObjectRegistry();
    registry.UnregisterObjectType<CameraImpl>();
}

SCENE_END_NAMESPACE()
