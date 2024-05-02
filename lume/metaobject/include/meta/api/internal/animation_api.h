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

#ifndef META_API_ANIMATION_API_H
#define META_API_ANIMATION_API_H

#include <base/containers/type_traits.h>

#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/intf_animation_modifier.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_named.h>

#include "object_api.h"

META_BEGIN_NAMESPACE()

namespace Internal {

/**
 * @brief IAnimation user API property forwarder.
 */
template<class FinalClass, const META_NS::ClassInfo& Class>
class AnimationInterfaceAPI : public ObjectInterfaceAPI<FinalClass, Class> {
    META_INTERFACE_API(AnimationInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(META_NS::IAnimation)
    META_API_OBJECT_CONVERTIBLE(META_NS::IAttachment)
    META_API_OBJECT_CONVERTIBLE(META_NS::INamed)
    META_API_CACHE_INTERFACE(META_NS::IAnimation, Animation)

public:
    META_API_INTERFACE_PROPERTY_CACHED(Animation, Name, BASE_NS::string)
    META_API_INTERFACE_PROPERTY_CACHED(Animation, Enabled, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Animation, Curve, META_NS::ICurve1D::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(Animation, Controller, META_NS::IAnimationController::WeakPtr)

    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Animation, TotalDuration, TimeSpan)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Animation, Running, bool)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Animation, Progress, float)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Animation, Valid, bool)

    using Super = ObjectInterfaceAPI<FinalClass, Class>;

    FinalClass& Parent(const META_NS::IStaggeredAnimation::Ptr& parent)
    {
        META_API_CACHED_INTERFACE(Animation)->SetParent(parent);
        return static_cast<FinalClass&>(*this);
    }

    /**
     * @brief See META_NS::IAnimation::Step()
     */
    void Step(const IClock::ConstPtr& clock)
    {
        META_API_CACHED_INTERFACE(Animation)->Step(clock);
    }
    /**
     * @brief See META_NS::IAnimation::Reset()
     */
    void Reset()
    {
        META_API_CACHED_INTERFACE(Animation)->Stop();
    }

    Event<IOnChanged> OnStarted()
    {
        return META_API_CACHED_INTERFACE(Animation)->OnStarted();
    }

    Event<IOnChanged> OnFinished()
    {
        return META_API_CACHED_INTERFACE(Animation)->OnFinished();
    }

    /**
     * @brief Attaches animation modifier to this animation.
     */
    FinalClass& Modifier(const IAnimationModifier::Ptr& modifier)
    {
        if (auto attachment = interface_pointer_cast<IAttachment>(modifier)) {
            return static_cast<FinalClass&>(Super::Attach(attachment));
        }

        return static_cast<FinalClass&>(*this);
    }
};

/**
 * @brief IPropertyAnimation user API property forwarder.
 */
template<class FinalClass, const META_NS::ClassInfo& Class>
class PropertyAnimationInterfaceAPI : public AnimationInterfaceAPI<FinalClass, Class> {
    META_INTERFACE_API(PropertyAnimationInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(META_NS::IPropertyAnimation)
    META_API_CACHE_INTERFACE(META_NS::IPropertyAnimation, PropertyAnimation)
    META_API_OBJECT_CONVERTIBLE(META_NS::ITimedAnimation)
    META_API_CACHE_INTERFACE(META_NS::ITimedAnimation, TimedAnimation)
public:
    META_API_INTERFACE_PROPERTY_CACHED(TimedAnimation, Duration, TimeSpan)

    inline Meta::Property<Meta::IProperty::WeakPtr> Property() noexcept
    {
        return GetPropertyAnimationInterface()->Property();
    }
    inline FinalClassType& Property(const Meta::IProperty::WeakPtr& value)
    {
        Property()->SetValue(value);
        return static_cast<FinalClassType&>(*this);
    }
};

/**
 * @brief IStartableAnimation user API property forwarder.
 */
template<class FinalClass, const META_NS::ClassInfo& Class>
class StartableAnimationInterfaceAPI : public AnimationInterfaceAPI<FinalClass, Class> {
    META_INTERFACE_API(StartableAnimationInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(META_NS::IStartableAnimation)
    META_API_CACHE_INTERFACE(META_NS::IStartableAnimation, StartableAnimation)
public:
    /**
     * @brief See META_NS::IStartableAnimation::Pause()
     */
    void Pause()
    {
        META_API_CACHED_INTERFACE(StartableAnimation)->Pause();
    }
    /**
     * @brief See META_NS::IStartableAnimation::Restart()
     */
    void Restart()
    {
        META_API_CACHED_INTERFACE(StartableAnimation)->Restart();
    }
    /**
     * @brief See META_NS::IStartableAnimation::Seek()
     */
    void Seek(float position)
    {
        META_API_CACHED_INTERFACE(StartableAnimation)->Seek(position);
    }
    /**
     * @brief See META_NS::IStartableAnimation::Start()
     */
    void Start()
    {
        META_API_CACHED_INTERFACE(StartableAnimation)->Start();
    }
    /**
     * @brief See META_NS::IStartableAnimation::Stop()
     */
    void Stop()
    {
        META_API_CACHED_INTERFACE(StartableAnimation)->Stop();
    }
};

template<class FinalClass, const META_NS::ClassInfo& Class>
class StartablePropertyAnimationInterfaceAPI : public StartableAnimationInterfaceAPI<FinalClass, Class> {
    META_INTERFACE_API(StartablePropertyAnimationInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(META_NS::IPropertyAnimation)
    META_API_CACHE_INTERFACE(META_NS::IPropertyAnimation, PropertyAnimation)
    META_API_OBJECT_CONVERTIBLE(META_NS::ITimedAnimation)
    META_API_CACHE_INTERFACE(META_NS::ITimedAnimation, TimedAnimation)
public:
    inline Meta::Property<Meta::IProperty::WeakPtr> Property() noexcept
    {
        return GetPropertyAnimationInterface()->Property();
    }
    inline FinalClassType& Property(const Meta::IProperty::WeakPtr& value)
    {
        Property()->SetValue(value);
        return static_cast<FinalClassType&>(*this);
    }
    META_API_INTERFACE_PROPERTY_CACHED(TimedAnimation, Duration, TimeSpan)
};

/**
 * @brief IStaggeredAnimation user API property forwarder.
 */
template<class FinalClass, const META_NS::ClassInfo& Class>
class StaggeredAnimationInterfaceAPI : public StartableAnimationInterfaceAPI<FinalClass, Class> {
    META_INTERFACE_API(StaggeredAnimationInterfaceAPI)
    META_API_OBJECT_CONVERTIBLE(META_NS::IContainer)
    META_API_CACHE_INTERFACE(META_NS::IContainer, Container)
    META_API_OBJECT_CONVERTIBLE(META_NS::IStaggeredAnimation)
    META_API_CACHE_INTERFACE(META_NS::IStaggeredAnimation, StaggeredAnimation)
public:
    FinalClassType& Child(const META_NS::IAnimation::Ptr& child)
    {
        META_API_CACHED_INTERFACE(Container)->Add(child);
        return static_cast<FinalClassType&>(*this);
    }

    BASE_NS::vector<META_NS::IAnimation::Ptr> GetAnimations()
    {
        return META_API_CACHED_INTERFACE(StaggeredAnimation)->GetAnimations();
    }
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_API_ANIMATION_API_H
