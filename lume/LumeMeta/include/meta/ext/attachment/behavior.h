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

#ifndef META_EXT_BEHAVIOR_H
#define META_EXT_BEHAVIOR_H

#include <meta/ext/attachment/attachment.h>
#include <meta/interface/intf_startable.h>

META_BEGIN_NAMESPACE()

namespace Internal {

/**
 * @brief The Behavior class is an helper class which takes care of common steps needed
 *        to implement the IStartable interface.
 *
 *        Startable objects (i.e. attachments which implement IStartable) are commonly referred to as behaviors. The
 *        runtime environment is usually implemented in such a way that the framework starts and stops any beheviors
 *        automatically as they become part of the hierarchy.
 */
class BehaviorBase : public IntroduceInterfaces<AttachmentFwd, IStartable> {
    META_OBJECT_NO_CLASSINFO(BehaviorBase, IntroduceInterfaces)

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(
        META_NS::IStartable, META_NS::StartBehavior, StartableMode, META_NS::StartBehavior::AUTOMATIC)
    META_STATIC_PROPERTY_DATA(META_NS::IStartable, META_NS::StartableState, StartableState,
        META_NS::StartableState::DETACHED, DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(META_NS::StartBehavior, StartableMode)
    META_IMPLEMENT_READONLY_PROPERTY(META_NS::StartableState, StartableState)
protected:
    /**
     * @brief Returns the state of the startable.
     */
    META_NS::StartableState GetState() const
    {
        return META_ACCESS_PROPERTY_VALUE(StartableState);
    }
    /**
     * @brief Called when the behavior has been attached to a target.
     *        If the behavior needs to do some initialization steps, it should override this
     *        function and do them here.
     * @return True if the operation completed successfully, false otherwise.
     */
    virtual bool OnAttach()
    {
        return true;
    };
    /**
     * @brief Called when the behavior is being detached from a target.
     */
    virtual void OnDetach() {};
    /**
     * @brief Called after the behavior has been successfully attached, and the runtime state of the framework is such
     *        that the behavior should start any processing.
     * @note  Usually it is enough to override OnStart/OnStop, OnAttach/OnDetach are not needed.
     * @return True if start was successful, false otherwise.
     */
    virtual bool OnStart()
    {
        return true;
    }
    /**
     * @brief Called when the behavior should stop any ongoing tasks.
     */
    virtual void OnStop() {}
    /**
     * @brief Returns a strong reference to the target object.
     */
    META_NS::IObject::Ptr GetObject() const
    {
        return object_.lock();
    }
    /**
     * @brief Returns a strong reference to the data context object.
     */
    META_NS::IObject::Ptr GetContext() const
    {
        return context_.lock();
    }
    /**
     * @brief Templated helper for GetObject, returning the target object casted to given type.
     */
    template<class T>
    typename T::Ptr GetObject() const
    {
        return interface_pointer_cast<T>(object_);
    }
    /**
     * @brief Templated helper for GetObject, returning the context object casted to given type.
     */
    template<class T>
    typename T::Ptr GetContext() const
    {
        return interface_pointer_cast<T>(context_);
    }

private: // META_NS::AttachmentFwd
    /** Private implementation of AttachmentFwd::AttachTo, handle properties and call the Attach of derived
     * implementation */
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override
    {
        auto object = interface_pointer_cast<META_NS::IObject>(target);
        if (!object) {
            CORE_LOG_E("Behavior target must implement IObject interface");
            return false;
        }
        object_ = object;
        context_ = dataContext;
        if (OnAttach()) {
            SetValue(META_ACCESS_PROPERTY(StartableState), META_NS::StartableState::ATTACHED);
            return true;
        }
        // Attach failed
        object_.reset();
        context_.reset();
        return false;
    }
    /** Private implementation of AttachmentFwd::DetachFrom, handle properties and call the Detach method of derived
     * implementation. */
    bool DetachFrom(const META_NS::IAttach::Ptr& /*target*/) override
    {
        OnDetach();
        SetValue(META_ACCESS_PROPERTY(StartableState), META_NS::StartableState::DETACHED);
        object_.reset();
        context_.reset();
        return true;
    }

    bool Start() override final
    {
        const auto state = META_ACCESS_PROPERTY_VALUE(StartableState);
        switch (state) {
            case StartableState::DETACHED: {
                CORE_LOG_E("Tried to start a detached behavior");
                break;
            }
            case StartableState::ATTACHED: {
                CORE_LOG_V("Calling OnStart");
                if (OnStart()) {
                    SetValue(META_ACCESS_PROPERTY(StartableState), StartableState::STARTED);
                    return true;
                }
                break;
            }
            case StartableState::STARTED: {
                CORE_LOG_V("Behavior already started");
                return true;
            }
        }
        return false;
    }
    bool Stop() override
    {
        const auto state = META_ACCESS_PROPERTY_VALUE(StartableState);
        if (state == StartableState::STARTED) {
            OnStop();
            SetValue(META_ACCESS_PROPERTY(StartableState), StartableState::ATTACHED);
            return true;
        }
        return false;
    }

private:
    META_NS::IObject::WeakPtr object_;
    META_NS::IObject::WeakPtr context_;
};

} // namespace Internal

/**
 * @brief The Behavior class is a helper class for implementing custom behaviors.
 *
 * @code
 *        META_REGISTER_CLASS(
 *        MyBehavior, "...", META_NS::ObjectCategoryBits::NO_CATEGORY, "My behavior")
 *
 *        class MyCustomBehavior : public META_NS::Behavior<IMyCustonInterface> {
 *            META_OBJECT(MyCustomBehavior, ClassId::MyBehavior, Behavior)
 *            ...
 *        };
 * @endcode
 */
template<class... Interfaces>
class Behavior : public IntroduceInterfaces<Internal::BehaviorBase, Interfaces...> {};

META_END_NAMESPACE()

#endif // META_EXT_BEHAVIOR_H
