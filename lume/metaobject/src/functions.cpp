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
#include "functions.h"

#include <meta/ext/serialization/serializer.h>

META_BEGIN_NAMESPACE()

BASE_NS::string SettableFunction::GetName() const
{
    auto p = func_.lock();
    return p ? p->GetName() : BASE_NS::string();
}
IObject::ConstPtr SettableFunction::GetDestination() const
{
    auto p = func_.lock();
    return p ? p->GetDestination() : nullptr;
}
void SettableFunction::Invoke(const ICallContext::Ptr& context) const
{
    if (auto p = func_.lock()) {
        p->Invoke(context);
    }
}
ICallContext::Ptr SettableFunction::CreateCallContext() const
{
    auto p = func_.lock();
    return p ? p->CreateCallContext() : nullptr;
}
bool SettableFunction::SetTarget(const IObject::Ptr& obj, BASE_NS::string_view name)
{
    return ResolveFunction(obj, name);
}
bool SettableFunction::ResolveFunction(const IObject::Ptr& obj, BASE_NS::string_view name)
{
    func_ = nullptr;
    if (auto metad = interface_cast<IMetadata>(obj)) {
        func_ = metad->GetFunctionByName(name);
    }
    return !func_.expired();
}
ReturnError SettableFunction::Export(IExportContext& c) const
{
    if (auto func = func_.lock()) {
        if (auto dest = func->GetDestination()) {
            return Serializer(c) & NamedValue("Destination", dest) & NamedValue("Function", func->GetName());
        }
        CORE_LOG_E("No destination object with Function");
    }
    return GenericError::FAIL;
}
ReturnError SettableFunction::Import(IImportContext& c)
{
    IObject::Ptr p;
    BASE_NS::string func;
    Serializer ser(c);
    if (ser & NamedValue("Destination", p) & NamedValue("Function", func)) {
        if (ResolveFunction(p, func)) {
            return GenericError::SUCCESS;
        }
    }
    return GenericError::FAIL;
}

BASE_NS::string PropertyFunction::GetName() const
{
    auto p = prop_.lock();
    return p ? p->GetName() : BASE_NS::string();
}
IObject::ConstPtr PropertyFunction::GetDestination() const
{
    return interface_pointer_cast<IObject>(prop_.lock());
}
void PropertyFunction::Invoke(const ICallContext::Ptr& context) const
{
    if (auto p = prop_.lock()) {
        context->SetResult(p->GetValue());
    } else {
        CORE_LOG_W("Invoked property function without valid property");
    }
}
ICallContext::Ptr PropertyFunction::CreateCallContext() const
{
    ICallContext::Ptr context;
    if (auto i = interface_pointer_cast<IPropertyInternalAny>(prop_)) {
        if (auto any = i->GetInternalAny()) {
            context = GetObjectRegistry().ConstructDefaultCallContext();
            if (context) {
                context->DefineResult(any->Clone(false));
            }
        }
    }
    return context;
}
bool PropertyFunction::SetTarget(const IProperty::ConstPtr& prop)
{
    prop_ = prop;
    return false;
}
ReturnError PropertyFunction::Export(IExportContext& c) const
{
    return Serializer(c) & NamedValue("source", prop_);
}
ReturnError PropertyFunction::Import(IImportContext& c)
{
    return Serializer(c) & NamedValue("source", uri_);
}
ReturnError PropertyFunction::Finalize(IImportFunctions& funcs)
{
    if (uri_.IsValid() && uri_.ReferencesProperty()) {
        auto objUri = uri_;
        objUri.TakeLastNode();
        if (auto obj = interface_pointer_cast<IMetadata>(funcs.ResolveRefUri(objUri))) {
            if (auto prop = obj->GetPropertyByName(uri_.ReferencedName())) {
                prop_ = prop;
            }
            return GenericError::SUCCESS;
        }
    }
    return GenericError::FAIL;
}

META_END_NAMESPACE()
