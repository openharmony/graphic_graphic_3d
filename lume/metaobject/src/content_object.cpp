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

#include <shared_mutex>

#include <meta/api/event_handler.h>
#include <meta/api/property/property_event_handler.h>
#include <meta/api/threading/mutex.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/intf_required_interfaces.h>
#include <meta/interface/loaders/intf_dynamic_content_loader.h>

#include "object.h"

META_BEGIN_NAMESPACE()

class ContentObject
    : public Internal::ObjectFwd<ContentObject, ClassId::ContentObject, IContent, IRequiredInterfaces, IIterable> {
    using Super = Internal::ObjectFwd<ContentObject, ClassId::ContentObject, IContent, IRequiredInterfaces, IIterable>;

public:
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IContent, IObject::Ptr, Content)
    META_IMPLEMENT_INTERFACE_PROPERTY(IContent, bool, ContentSearchable, true)
    META_IMPLEMENT_INTERFACE_PROPERTY(IContent, IContentLoader::Ptr, ContentLoader)

    bool SetContent(const IObject::Ptr& content) override
    {
        META_ACCESS_PROPERTY(ContentLoader)->SetValue(nullptr);
        return SetContentInternal(content);
    }

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = Super::Build(data);
        if (ret) {
            loaderChanged_.Subscribe(META_ACCESS_PROPERTY(ContentLoader), [this] { OnLoaderChanged(); });
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

    void OnLoaderChanged()
    {
        contentChanged_.Unsubscribe();
        if (auto dynamic = interface_pointer_cast<IDynamicContentLoader>(META_ACCESS_PROPERTY_VALUE(ContentLoader))) {
            // If our loader is dynamic (i.e. the content can change), subscribe to change events
            contentChanged_.Subscribe<IOnChanged>(dynamic->ContentChanged(), [&] { OnContentChanged(); });
        }
        OnContentChanged();
    }

    void OnContentChanged()
    {
        const auto loader = META_ACCESS_PROPERTY_VALUE(ContentLoader);
        SetContentInternal(loader ? loader->Create({}) : nullptr);
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
        bool valid = false;
        if (const auto content = META_ACCESS_PROPERTY_VALUE(Content)) {
            valid = CheckContentRequirements(content);
        }
        if (!valid) {
            // We don't have valid content, check if we could create one with our loader
            OnContentChanged();
        }
        return true;
    }
    BASE_NS::vector<TypeId> GetRequiredInterfaces() const override
    {
        std::shared_lock lock(mutex_);
        return requiredInterfaces_;
    }

    bool SetContentInternal(const IObject::Ptr& content)
    {
        bool valid = CheckContentRequirements(content);
        META_ACCESS_PROPERTY(Content)->SetValue(valid ? content : nullptr);
        if (!valid) {
            CORE_LOG_W("Content does not fulfil interface requirements");
        }
        return valid;
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
    PropertyChangedEventHandler loaderChanged_;
    EventHandler contentChanged_;
    BASE_NS::vector<TypeId> requiredInterfaces_;
    mutable std::shared_mutex mutex_;
};

namespace Internal {

IObjectFactory::Ptr GetContentObjectFactory()
{
    return ContentObject::GetFactory();
}

} // namespace Internal

META_END_NAMESPACE()
