/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Implementation of IContent interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-09-06
 */

#include <mutex>
#include <shared_mutex>

#include <meta/api/event_handler.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/api/threading/mutex.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_required_interfaces.h>
#include <meta/interface/loaders/intf_dynamic_content_loader.h>

#include "object.h"

META_BEGIN_INTERNAL_NAMESPACE()

class ContentObject : public IntroduceInterfaces<MetaObject, IContent, IRequiredInterfaces, IIterable> {
    META_OBJECT(ContentObject, ClassId::ContentObject, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IContent, IObject::Ptr, Content)
    META_STATIC_PROPERTY_DATA(IContent, bool, ContentSearchable, true)
    META_END_STATIC_DATA()
    META_IMPLEMENT_READONLY_PROPERTY(IObject::Ptr, Content)
    META_IMPLEMENT_PROPERTY(bool, ContentSearchable)

    bool SetContent(const IObject::Ptr& content) override
    {
        bool valid = CheckContentRequirements(content);
        SetValue(META_ACCESS_PROPERTY(Content), valid ? content : nullptr);
        if (!valid) {
            CORE_LOG_W("Content does not fulfil interface requirements");
        }
        return valid;
    }

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = Super::Build(data);
        if (ret) {
            OnSerializeChanged();
        }
        return ret;
    }

    void Destroy() override
    {
        if (auto c = META_ACCESS_PROPERTY(Content)) {
            c->OnChanged()->Reset();
            c->SetValue(nullptr);
        }
        Super::Destroy();
    }

    void OnSerializeChanged()
    {
        if (auto cont = META_ACCESS_PROPERTY(Content)) {
            META_NS::SetObjectFlags(cont.GetProperty(), ObjectFlagBits::SERIALIZE,
                GetObjectFlags().IsSet(ObjectFlagBits::SERIALIZE_HIERARCHY));
        }
    }

    void SetObjectFlags(const ObjectFlagBitsValue& flags) override
    {
        Super::SetObjectFlags(flags);
        OnSerializeChanged();
    }

    ObjectFlagBitsValue GetObjectDefaultFlags() const override
    {
        auto flags = Super::GetObjectDefaultFlags();
        flags &= ~ObjectFlagBitsValue(ObjectFlagBits::SERIALIZE_HIERARCHY);
        return flags;
    }

    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces) override
    {
        {
            std::unique_lock lock(mutex_);
            requiredInterfaces_ = interfaces;
        }
        if (const auto content = META_ACCESS_PROPERTY_VALUE(Content)) {
            if (!CheckContentRequirements(content)) {
                SetContent(nullptr);
            }
        }
        return true;
    }
    BASE_NS::vector<TypeId> GetRequiredInterfaces() const override
    {
        std::shared_lock lock(mutex_);
        return requiredInterfaces_;
    }

    bool CheckContentRequirements(const IObject::Ptr& object)
    {
        std::shared_lock lock(mutex_);
        // Null object always passes content requirements
        return !object || CheckInterfaces(object, requiredInterfaces_, true);
    }

    template<typename Func>
    IterationResult IterateImpl(const Func& f) const
    {
        if (!f) {
            CORE_LOG_W("Incompatible function with Iterate");
            return IterationResult::FAILED;
        }
        if (META_ACCESS_PROPERTY_VALUE(ContentSearchable)) {
            if (auto c = META_ACCESS_PROPERTY_VALUE(Content)) {
                return f->Invoke(c);
            }
        }
        return IterationResult::CONTINUE;
    }

    IterationResult Iterate(const IterationParameters& params) override
    {
        return IterateImpl(params.function.GetInterface<IIterableCallable<IObject::Ptr>>());
    }

    IterationResult Iterate(const IterationParameters& params) const override
    {
        return IterateImpl(params.function.GetInterface<IIterableConstCallable<IObject::Ptr>>());
    }

private:
    mutable std::shared_mutex mutex_;
    BASE_NS::vector<TypeId> requiredInterfaces_;
};

IObjectFactory::Ptr GetContentObjectFactory()
{
    return ContentObject::GetFactory();
}

META_END_INTERNAL_NAMESPACE()
