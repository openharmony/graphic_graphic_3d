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

#ifndef META_API_ANIMATION_H
#define META_API_ANIMATION_H

#include <meta/api/curves/easing_curve.h>
#include <meta/api/internal/animation_api.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The SequentialAnimation class provides a wrapper for ISequentialAnimation.
 */
class SequentialAnimation final
    : public Internal::StaggeredAnimationInterfaceAPI<SequentialAnimation, META_NS::ClassId::SequentialAnimation> {
    META_API(SequentialAnimation)
};

/**
 * @brief The ParallelAnimation class provides a wrapper for IParallelAnimation.
 */
class ParallelAnimation final
    : public Internal::StaggeredAnimationInterfaceAPI<ParallelAnimation, META_NS::ClassId::ParallelAnimation> {
    META_API(ParallelAnimation)
};

/**
 * @brief The PropertyAnimation class provides a wrapper for IPropertyAnimation.
 */
class PropertyAnimation final
    : public Internal::PropertyAnimationInterfaceAPI<PropertyAnimation, META_NS::ClassId::PropertyAnimation> {
    META_API(PropertyAnimation)
};

/**
 * @brief The KeyframeAnimation<T> class provides a wrapper for IKeyframeAnimation,
 *        with automatic typing for From and To property.
 */
template<class Type>
class KeyframeAnimation final : public Internal::StartablePropertyAnimationInterfaceAPI<KeyframeAnimation<Type>,
                                    META_NS::ClassId::KeyframeAnimation> {
    META_API(KeyframeAnimation<Type>)
    META_API_OBJECT_CONVERTIBLE(META_NS::IKeyframeAnimation)
    META_API_CACHE_INTERFACE(META_NS::IKeyframeAnimation, KeyframeAnimation)
public:
    /** Creates the contained object */
    static META_NS::IObject::Ptr Create()
    {
        auto object = META_NS::GetObjectRegistry().Create(META_NS::ClassId::KeyframeAnimation);
        return object;
    }
    META_API_INTERFACE_PROPERTY_CACHED(KeyframeAnimation, From, IAny::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(KeyframeAnimation, To, IAny::Ptr)
    /** Get the starting value of the animation */
    auto& From(const Type& value)
    {
        if (auto p = META_API_CACHED_INTERFACE(KeyframeAnimation)->From()) {
            if (auto pv = p->GetValue()) { // IAny::Ptr
                pv->SetValue(value);
            } else {
                p->SetValue(IAny::Ptr { new Any<Type>(value) });
            }
        }
        return *this;
    }
    /** Set the target value of the animation */
    auto& To(const Type& value)
    {
        if (auto p = META_API_CACHED_INTERFACE(KeyframeAnimation)->To()) {
            if (auto pv = p->GetValue()) { // IAny::Ptr
                pv->SetValue(value);
            } else {
                p->SetValue(IAny::Ptr { new Any<Type>(value) });
            }
        }
        return *this;
    }
};

/**
 * @brief The TrackAnimation<T> class provides a wrapper for ITrackAnimation,
 *        with automatic typing for key-frames.
 */
template<class Type>
class TrackAnimation final
    : public Internal::StartablePropertyAnimationInterfaceAPI<TrackAnimation<Type>, META_NS::ClassId::TrackAnimation> {
    META_API(TrackAnimation<Type>)
    META_API_OBJECT_CONVERTIBLE(META_NS::ITrackAnimation)
    META_API_CACHE_INTERFACE(META_NS::ITrackAnimation, TrackAnimation)
public:
    /** Creates the contained object */
    static META_NS::IObject::Ptr Create()
    {
        const auto object = META_NS::GetObjectRegistry().Create(META_NS::ClassId::TrackAnimation);
        if (auto track = interface_cast<META_NS::ITrackAnimation>(object)) {
            if (auto internal = interface_cast<META_NS::IPropertyInternalAny>(track->Keyframes())) {
                internal->SetInternalAny(ArrayAny<Type>({}).Clone(false));
            }
        }
        return object;
    }
    META_API_INTERFACE_ARRAY_PROPERTY_CACHED(TrackAnimation, KeyframeCurves, META_NS::ICurve1D::Ptr)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(TrackAnimation, CurrentKeyframeIndex, uint32_t)
    /** Get the track timestamps */
    META_NS::ArrayProperty<float> Timestamps()
    {
        return META_API_CACHED_INTERFACE(TrackAnimation)->Timestamps();
    }
    /** Set the track timestamps */
    auto& Timestamps(BASE_NS::array_view<float> timestamps)
    {
        Timestamps()->SetValue(BASE_NS::move(timestamps));
        return *this;
    }
    /** Get the track key-frames */
    META_NS::ArrayProperty<Type> Keyframes()
    {
        return META_NS::ArrayProperty<Type>(META_API_CACHED_INTERFACE(TrackAnimation)->Keyframes());
    }
    /** Set the track key-frames */
    auto& Keyframes(BASE_NS::array_view<Type> keyframes)
    {
        if (auto kf = Keyframes()) {
            kf->SetValue(BASE_NS::move(keyframes));
        }
        return *this;
    }
    /** Get the track timestamps */
    META_NS::ArrayProperty<IFunction::Ptr> KeyframeHandlers()
    {
        return META_API_CACHED_INTERFACE(TrackAnimation)->KeyframeHandlers();
    }
    auto& KeyframeHandlers(BASE_NS::array_view<IFunction::Ptr> handlers)
    {
        KeyframeHandlers()->SetValue(BASE_NS::move(handlers));
        return *this;
    }
    size_t AddKeyframe(float timestamp, const IAny::ConstPtr& value)
    {
        return META_API_CACHED_INTERFACE(TrackAnimation)->AddKeyframe(timestamp, value);
    }
    bool RemoveKeyframe(size_t index)
    {
        return META_API_CACHED_INTERFACE(TrackAnimation)->RemoveKeyframe(index);
    }
    void RemoveAllKeyframes()
    {
        return META_API_CACHED_INTERFACE(TrackAnimation)->RemoveAllKeyframes();
    }
};

META_END_NAMESPACE()

#endif // META_API_ANIMATION_H
