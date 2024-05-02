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
#include "connector.h"

#include <meta/ext/serialization/serializer.h>

META_BEGIN_NAMESPACE()

bool Connector::AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext)
{
    auto source = source_.lock();
    auto dest = interface_cast<IMetadata>(target);
    if (!source || !dest) {
        CORE_LOG_E("Failed to attach Connector");
        return false;
    }

    auto event = source->GetEventByName(eventName_);
    if (!event) {
        CORE_LOG_E("Failed to attach Connector: No such event");
        return false;
    }

    auto func = dest->GetFunctionByName(funcName_);
    if (!func) {
        CORE_LOG_E("Failed to attach Connector: No such function");
        return false;
    }

    handle_ = event->AddHandler(func);
    return handle_ != IEvent::Token {};
}

bool Connector::DetachFrom(const META_NS::IAttach::Ptr& target)
{
    if (handle_) {
        auto source = source_.lock();
        if (source) {
            auto event = source->GetEventByName(eventName_);
            if (!event) {
                return false;
            }
            event->RemoveHandler(handle_);
            handle_ = IEvent::Token {};
        }
    }
    return true;
}

bool Connector::Connect(const IObject::Ptr& source, const BASE_NS::string& event, const BASE_NS::string& func)
{
    source_ = interface_pointer_cast<IMetadata>(source);
    eventName_ = event;
    funcName_ = func;
    return !source_.expired() && !eventName_.empty() && !funcName_.empty();
}

IObject::Ptr Connector::GetSource() const
{
    return interface_pointer_cast<IObject>(source_);
}
BASE_NS::string Connector::GetEventName() const
{
    return eventName_;
}
BASE_NS::string Connector::GetFunctionName() const
{
    return funcName_;
}

ReturnError Connector::Export(IExportContext& c) const
{
    return Serializer(c) & NamedValue("Source", source_) & NamedValue("Event", eventName_) &
           NamedValue("Function", funcName_);
}
ReturnError Connector::Import(IImportContext& c)
{
    IMetadata::Ptr p;
    Serializer ser(c);
    ser& NamedValue("Source", p) & NamedValue("Event", eventName_) & NamedValue("Function", funcName_);
    if (ser && p) {
        source_ = p;
    }
    return ser;
}

META_END_NAMESPACE()
