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
#ifndef SCENE_PLUGIN_ECS_ANIMATION_H
#define SCENE_PLUGIN_ECS_ANIMATION_H

#undef InterlockedIncrement
#undef InterlockedDecrement

#include <optional>
#include <scene_plugin/interface/intf_ecs_animation.h>

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/name_component.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>

#include <meta/base/shared_ptr.h>
#include <meta/ext/animation/interpolator.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/ext/object_container.h>
#include <meta/ext/event_impl.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/intf_container.h>

#include "scene_holder.h"

SCENE_BEGIN_NAMESPACE()

class EcsTrackAnimation final : public META_NS::ObjectFwd<EcsTrackAnimation, ClassId::EcsTrackAnimation, META_NS::ClassId::Object,
          IEcsTrackAnimation, META_NS::IStartableAnimation, META_NS::ITrackAnimation, META_NS::IPropertyAnimation,
          META_NS::IContainable, META_NS::IMutableContainable, SCENE_NS::IEcsProxyObject, META_NS::ITimedAnimation> {
public:
    // From INamed
    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

    // From ITrackAnimation
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(ITrackAnimation, float, Timestamps, {})
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(ITrackAnimation, META_NS::IFunction::Ptr, KeyframeHandlers)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(ITrackAnimation, uint32_t, CurrentKeyframeIndex, -1)
    META_IMPLEMENT_INTERFACE_PROPERTY(IPropertyAnimation, META_NS::IProperty::WeakPtr, Property, {})
    META_IMPLEMENT_INTERFACE_ARRAY_PROPERTY(ITrackAnimation, META_NS::ICurve1D::Ptr, KeyframeCurves, {})
    META_NS::IProperty::Ptr Keyframes() const override;
    size_t AddKeyframe(float timestamp, const META_NS::IAny::ConstPtr& value) override;
    bool RemoveKeyframe(size_t index) override;
    void RemoveAllKeyframes() override;

    // From IAnimation
    META_IMPLEMENT_INTERFACE_PROPERTY(IAnimation, bool, Enabled, true)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, bool, Valid, false, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, META_NS::TimeSpan, TotalDuration,
        META_NS::TimeSpan::Milliseconds(500), META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, bool, Running, false, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, float, Progress, 0, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_PROPERTY(IAnimation, META_NS::ICurve1D::Ptr, Curve)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        IAnimation, META_NS::IAnimationController::WeakPtr, Controller, {}, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)

    // From IAnimationWithModifiableDuration
    META_IMPLEMENT_INTERFACE_PROPERTY(IAnimation, META_NS::TimeSpan, Duration, META_NS::TimeSpan::Milliseconds(500))

    bool Build(const META_NS::IMetadata::Ptr& data) override;

    void Seek(float position) override;
    void Step(const META_NS::IClock::ConstPtr& clock) override;
    void Start() override;
    void Stop() override;
    void Pause() override;
    void Restart() override;
    void Finish() override;

    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnFinished)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnStarted)

    void SetEntity(CORE_NS::Entity entity) override
    {
        entity_ = entity;
        if (auto ecsListener = ecsListener_.lock()) {
            if (auto animationTrackManager =
                    CORE_NS::GetManager<CORE3D_NS::IAnimationTrackComponentManager>(*ecsListener->ecs_)) {
                ecsListener->AddEntity(entity, GetSelf<SCENE_NS::IEcsProxyObject>(), *animationTrackManager);
            }
            if (auto animationInputManager =
                    CORE_NS::GetManager<CORE3D_NS::IAnimationInputComponentManager>(*ecsListener->ecs_)) {
                ecsListener->AddEntity(entity, GetSelf<SCENE_NS::IEcsProxyObject>(), *animationInputManager);
            }
        }
    }

public: // from IEcsProxyObject
    void SetCommonListener(BASE_NS::shared_ptr<SCENE_NS::EcsListener> ecsListener) override
    {
        ecsListener_ = ecsListener;
    }

    void DoEntityEvent(CORE_NS::IEcs::EntityListener::EventType type, const CORE_NS::Entity& entity) override
    {
        if (auto parent = interface_pointer_cast<SCENE_NS::IEcsProxyObject>(GetParent())) {
            parent->DoEntityEvent(type, entity);
        }
    }

    void DoComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
        const CORE_NS::IComponentManager& componentManager, const CORE_NS::Entity& entity) override
    {
        if (auto parent = interface_pointer_cast<SCENE_NS::IEcsProxyObject>(GetParent())) {
            parent->DoComponentEvent(type, componentManager, entity);
        }
    }

    CORE_NS::Entity GetEntity() const override
    {
        return entity_;
    }

    void SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& super) override
    {
        ObjectFwd::SetSuperInstance(aggr, super);
        containable_ = interface_cast<IContainable>(super);
        mutableContainable_ = interface_cast<IMutableContainable>(super);
    }

    void SetParent(const IObject::Ptr& parent) override
    {
        if (mutableContainable_) {
            mutableContainable_->SetParent(parent);
        } else {
            parent_ = parent;
        }
    }

    IObject::Ptr GetParent() const override
    {
        if (containable_) {
            return containable_->GetParent();
        }
        return parent_.lock();
    }

    void Destroy() override
    {
        if (auto ecsListener = ecsListener_.lock()) {
            if (!ecsListener->ecs_) {
                return;
            }
            if (auto animationTrackManager =
                    CORE_NS::GetManager<CORE3D_NS::IAnimationTrackComponentManager>(*ecsListener->ecs_)) {
                ecsListener->RemoveEntity(entity_, GetSelf<SCENE_NS::IEcsProxyObject>(), *animationTrackManager);
            }
            if (auto animationInputManager =
                    CORE_NS::GetManager<CORE3D_NS::IAnimationInputComponentManager>(*ecsListener->ecs_)) {
                ecsListener->RemoveEntity(entity_, GetSelf<SCENE_NS::IEcsProxyObject>(), *animationInputManager);
            }
        }
    }

private:
    CORE_NS::Entity entity_;
    BASE_NS::string name_;
    META_NS::IProperty::Ptr keyframes_;

    mutable META_NS::EventImpl<META_NS::IOnChanged> onFinished_;
    mutable META_NS::EventImpl<META_NS::IOnChanged> onStarted_;

    BASE_NS::weak_ptr<SCENE_NS::EcsListener> ecsListener_;
    META_NS::IContainable* containable_ {};
    META_NS::IMutableContainable* mutableContainable_ {};
    META_NS::IObject::WeakPtr parent_;
};

class EcsAnimation final : public META_NS::ObjectContainerFwd<EcsAnimation, ClassId::EcsAnimation, IEcsAnimation,
                                META_NS::IParallelAnimation, META_NS::ITimedAnimation, META_NS::IStartableAnimation,
                               SCENE_NS::IEcsProxyObject, META_NS::IAttachment> {
public:
    // From IEcsAnimation
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(
        IEcsAnimation, bool, ReadOnly, false, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)

    // From INamed
    META_IMPLEMENT_INTERFACE_PROPERTY(INamed, BASE_NS::string, Name, {})

    // From IAnimation
    META_IMPLEMENT_INTERFACE_PROPERTY(IAnimation, bool, Enabled, true)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, bool, Valid, false, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, META_NS::TimeSpan, TotalDuration,
        META_NS::TimeSpan::Milliseconds(500), META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, bool, Running, false, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, float, Progress, 0, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_PROPERTY(IAnimation, META_NS::ICurve1D::Ptr, Curve)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        IAnimation, META_NS::IAnimationController::WeakPtr, Controller, {}, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_READONLY_PROPERTY(META_NS::IObject::WeakPtr, DataContext)
    META_IMPLEMENT_READONLY_PROPERTY(META_NS::IAttach::WeakPtr, AttachedTo)

    META_IMPLEMENT_INTERFACE_PROPERTY(
        ITimedAnimation, META_NS::TimeSpan, Duration, META_NS::TimeSpan::Milliseconds(500))

    void AddAnimation(const IAnimation::Ptr&) override;
    void RemoveAnimation(const IAnimation::Ptr&) override;

    BASE_NS::vector<IAnimation::Ptr> GetAnimations() const override;

public:
    void SetSceneHolder(SceneHolder::Ptr& sceneHolder)
    {
        sceneHolder_ = sceneHolder;
        SetCommonListener(sceneHolder->GetCommonEcsListener());
    }

    SceneHolder::Ptr GetSceneHolder()
    {
        return sceneHolder_.lock();
    }

    // from IEcsProxyObject
    void SetCommonListener(BASE_NS::shared_ptr<SCENE_NS::EcsListener> ecsListener) override
    {
        ecsListener_ = ecsListener;
    }
    void DoEntityEvent(CORE_NS::IEcs::EntityListener::EventType, const CORE_NS::Entity& entity) override {}
    void DoComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
        const CORE_NS::IComponentManager& componentManager, const CORE_NS::Entity& entity) override;

    // From Object
    BASE_NS::string GetName() const override;

    bool SetRootEntity(CORE_NS::Entity entity) override;
    CORE_NS::Entity GetRootEntity() const override;

    void SetEntity(CORE_NS::IEcs& ecs, CORE_NS::Entity entity) override;
    CORE_NS::Entity GetEntity() const override;

    void SetDuration(uint32_t ms) override;

    bool Retarget(CORE_NS::Entity entity) override;

    void Seek(float position) override;
    void Step(const META_NS::IClock::ConstPtr& clock) override;
    void Start() override;
    void Stop() override;
    void Pause() override;
    void Restart() override;
    void Finish() override;

    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnFinished)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnStarted)

    void AddKey(IEcsTrackAnimation::Ptr track, float time) override;
    void RemoveKey(IEcsTrackAnimation::Ptr track, uint32_t index) override;
    void UpdateKey(IEcsTrackAnimation::Ptr track, uint32_t oldKeyIndex, uint32_t newKeyIndex, float time) override;

    IEcsTrackAnimation::Ptr CreateAnimationTrack(
        CORE_NS::Entity rootEntity, CORE_NS::Entity target, BASE_NS::string_view property) override;
    IEcsTrackAnimation::Ptr GetAnimationTrack(CORE_NS::Entity target, BASE_NS::string_view property) override;
    void DestroyAnimationTrack(IEcsTrackAnimation::Ptr track) override;
    void DestroyAllAnimationTracks() override;
    void Destroy() override;

    BASE_NS::vector<CORE_NS::EntityReference> GetAllRelatedEntities() const override;

public: // ISerialization
    //todo
    //bool Export(
    //    META_NS::Serialization::IExportContext& context, META_NS::Serialization::ClassPrimitive& value) const override;

    //bool Import(
    //    META_NS::Serialization::IImportContext& context, const META_NS::Serialization::ClassPrimitive& value) override;

    /**
     * @brief Called by the framework when an the attachment is being attached to an IObject. If this
     *        function succeeds, the object is attached to the target.
     * @param object The IObject instance the attachment is attached to.
     * @param dataContext The data context for this attachment.
     * @note The data context can be the same object as the object being attached to, or
     *       something else. It is up to the attachment to decide how to handle them.
     * @return The implementation should return true if the attachment can be attached to target object.
     *         If the attachment cannot be added, the implementation should return false.
     */
    bool Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        META_ACCESS_PROPERTY(AttachedTo)->SetValue(target);
        META_ACCESS_PROPERTY(DataContext)->SetValue(dataContext);
        return true;
    }
    /**
     * @brief Detach the attachment from an object.
     * @param object The object to attach to.
     * @return If the attachment can be detached from the target, the implementation should return true.
     *         If detaching is not possible, the implementation should return false. In such a case the
     *         target may choose to not remove the attachment. During for example object destruction,
     *         the target will ignore the return value.
     */
    bool Detaching(const IAttach::Ptr& target) override
    {
        META_ACCESS_PROPERTY(AttachedTo)->SetValue({});
        META_ACCESS_PROPERTY(DataContext)->SetValue({});
        return true;
    }

protected:
    bool Build(const META_NS::IMetadata::Ptr& data) override;

private:
    struct Data {
        inline explicit operator bool()
        {
            return (property != nullptr) && !data.empty();
        };

        const CORE_NS::PropertyTypeDecl* property { nullptr };
        BASE_NS::vector<uint8_t> data;
    };

    CORE_NS::Entity TryResolveAnimationRoot();

    Data GetProperty(BASE_NS::Uid componentUid, CORE_NS::Entity entity, BASE_NS::string property) const;
    void SetKeyFrameData(CORE_NS::Entity animationTrack, float timeStamp, BASE_NS::vector<uint8_t> valueData);
    void UpdateTimestamps(IEcsTrackAnimation& track, CORE_NS::Entity timestampEntity);

    // Return the highest timestamp from the keyframes of the animation track.
    float GetTrackDuration(CORE_NS::Entity animationTrack);
    void UpdateAnimationTrackDuration(CORE_NS::Entity animationTrack);

    void SetProgress(float progress);
    void SetTime(uint32_t value);

    void OnDestroyAnimationTrack(IEcsTrackAnimation::Ptr track);

    void OnAnimationStateChanged(CORE_NS::IEcs::ComponentListener::EventType event);
    void OnAnimationNameChanged(CORE_NS::IEcs::ComponentListener::EventType event);
    void OnAnimationChanged(CORE_NS::IEcs::ComponentListener::EventType event);
    void OnAnimationTracksChanged(CORE_NS::IEcs::ComponentListener::EventType event, CORE_NS::Entity entity);
    void OnAnimationInputsChanged(CORE_NS::IEcs::ComponentListener::EventType event, CORE_NS::Entity entity);

    void OnNamePropertyChanged();
    void OnDurationPropertyChanged();
    void OnProgressPropertyChanged();

    void OnAnimationTrackChanged(IEcsTrackAnimation& track, CORE_NS::Entity trackEntity);
    void OnAnimationTimestampsChanged(IEcsTrackAnimation& track, CORE_NS::Entity timestampEntity);

    bool IsAnimationTrackArrayModified();
    void GatherAnimationTracks();

    CORE_NS::IEcs* ecs_ { nullptr };
    CORE_NS::EntityReference entity_ {};
    CORE_NS::Entity root_ {};

    CORE3D_NS::IAnimationComponentManager* animationManager_ { nullptr };
    CORE3D_NS::IAnimationTrackComponentManager* animationTrackManager_ { nullptr };
    CORE3D_NS::IAnimationInputComponentManager* animationInputManager_ { nullptr };
    CORE3D_NS::IAnimationOutputComponentManager* animationOutputManager_ { nullptr };
    CORE_NS::IComponentManager* animationStateManager_ { nullptr };
    CORE3D_NS::INameComponentManager* nameManager_ { nullptr };

    bool updateGuard_ { false };

    int32_t repeatCount_ { 1 };
    std::optional<META_NS::TimeSpan> lastFrameTime_ {};

    mutable META_NS::EventImpl<META_NS::IOnChanged> onFinished_;
    mutable META_NS::EventImpl<META_NS::IOnChanged> onStarted_;

    SceneHolder::WeakPtr sceneHolder_;
    BASE_NS::weak_ptr<SCENE_NS::EcsListener> ecsListener_;
};

void RegisterEcsAnimationObjectType();
void UnregisterEcsAnimationObjectType();

SCENE_END_NAMESPACE()

#endif // SCENE_PLUGIN_ECS_ANIMATION_H
