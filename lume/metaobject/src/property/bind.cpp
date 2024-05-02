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
#include "bind.h"

#include <meta/ext/serialization/serializer.h>

#include "../any.h"
#include "dependencies.h"

META_BEGIN_NAMESPACE()
namespace Internal {

Bind::~Bind()
{
    for (auto it = dependencies_.begin(); it != dependencies_.end(); ++it) {
        if (auto p = it->lock()) {
            p->OnChanged()->RemoveHandler(uintptr_t(this));
        }
    }
}

AnyReturnValue Bind::SetValue(const IAny& value)
{
    return AnyReturn::NOT_SUPPORTED;
}
const IAny& Bind::GetValue() const
{
    if (func_ && context_) {
        context_->Reset();
        func_->Invoke(context_);
        if (context_->Succeeded()) {
            return *context_->GetResult();
        }
    }
    CORE_LOG_W("GetValue called for invalid bind");
    return INVALID_ANY;
}
bool Bind::IsCompatible(const TypeId& id) const
{
    return context_ && context_->GetResult() && META_NS::IsCompatible(*context_->GetResult(), id);
}
bool Bind::SetTarget(const IProperty::ConstPtr& prop, bool getDeps, const IProperty* owner)
{
    auto f = GetObjectRegistry().Create<IFunction>(META_NS::ClassId::PropertyFunction);
    if (auto i = interface_cast<IPropertyFunction>(f)) {
        i->SetTarget(prop);
        if (SetTarget(f, getDeps, owner)) {
            return AddDependency(prop);
        }
    }
    return false;
}
bool Bind::SetTarget(const IFunction::ConstPtr& func, bool getDeps, const IProperty* owner)
{
    // inherit serializability from the function
    META_NS::SetObjectFlags(static_cast<IObjectFlags*>(this), ObjectFlagBits::SERIALIZE,
        META_NS::IsFlagSet(func, ObjectFlagBits::SERIALIZE));
    func_ = func;
    return func_ && CreateContext(getDeps, owner);
}
IFunction::ConstPtr Bind::GetTarget() const
{
    return func_;
}
bool Bind::AddDependency(const INotifyOnChange::ConstPtr& dep)
{
    for (auto& d : dependencies_) {
        if (dep == d.lock()) {
            return true;
        }
    }
    dep->OnChanged()->AddHandler(event_, uintptr_t(this));
    dependencies_.push_back(dep);
    return true;
}
bool Bind::RemoveDependency(const INotifyOnChange::ConstPtr& dep)
{
    for (auto it = dependencies_.begin(); it != dependencies_.end(); ++it) {
        if (it->lock() == dep) {
            dep->OnChanged()->RemoveHandler(uintptr_t(this));
            dependencies_.erase(it);
            return true;
        }
    }
    return false;
}
BASE_NS::vector<INotifyOnChange::ConstPtr> Bind::GetDependencies() const
{
    BASE_NS::vector<INotifyOnChange::ConstPtr> deps;
    for (auto&& v : dependencies_) {
        if (auto d = v.lock()) {
            deps.push_back(d);
        }
    }
    return deps;
}
BASE_NS::shared_ptr<IEvent> Bind::EventOnChanged() const
{
    return event_;
}
ReturnError Bind::Export(IExportContext& c) const
{
    return Serializer(c) & NamedValue("function", func_);
}
ReturnError Bind::Import(IImportContext& c)
{
    return Serializer(c) & NamedValue("function", func_);
}
ReturnError Bind::Finalize(IImportFunctions&)
{
    if (func_) {
        if (CreateContext(false, nullptr)) {
            if (auto i = interface_cast<IPropertyFunction>(func_)) {
                if (auto noti = interface_pointer_cast<INotifyOnChange>(i->GetDestination())) {
                    AddDependency(noti);
                }
            }
            return GenericError::SUCCESS;
        }
    }
    return GenericError::FAIL;
}

bool Bind::CreateContext(bool eval, const IProperty* owner)
{
    context_ = func_->CreateCallContext();
    if (context_ && eval) {
        BASE_NS::vector<IProperty::ConstPtr> deps;

        auto& d = GetDeps();
        d.Start();
        // Evaluate to collect dependencies
        GetValue();
        auto state = d.GetImmediateDependencies(deps);
        if (state && owner && d.HasDependency(owner)) {
            state = GenericError::RECURSIVE_CALL;
        }
        d.End();
        if (!state) {
            return false;
        }
        for (auto&& v : deps) {
            if (auto noti = interface_pointer_cast<INotifyOnChange>(v)) {
                AddDependency(noti);
            }
        }
    }
    return context_ != nullptr;
}

void Bind::Reset() {}

} // namespace Internal
META_END_NAMESPACE()