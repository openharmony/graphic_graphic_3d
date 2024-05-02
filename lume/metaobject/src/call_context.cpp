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
#include "call_context.h"

META_BEGIN_NAMESPACE()

DefaultCallContext::~DefaultCallContext() = default;

DefaultCallContext::DefaultCallContext() = default;

DefaultCallContext::DefaultCallContext(const DefaultCallContext& other) noexcept : succeeded_(other.succeeded_)
{
    if (auto p = interface_cast<ICloneable>(other.result_)) {
        result_ = interface_pointer_cast<IAny>(p->GetClone());
    }
    params_.resize(other.params_.size());
    for (int i = 0; i != params_.size(); ++i) {
        params_[i].name = other.params_[i].name;
        if (auto p = interface_cast<ICloneable>(other.params_[i].value)) {
            params_[i].value = interface_pointer_cast<IAny>(p->GetClone());
        }
    }
}

DefaultCallContext::DefaultCallContext(DefaultCallContext&& other) noexcept
    : params_(std::move(other.params_)), succeeded_(other.succeeded_), result_(std::move(other.result_))
{}

DefaultCallContext& DefaultCallContext::operator=(const DefaultCallContext& other) noexcept
{
    if (&other == this) {
        return *this;
    }
    if (auto p = interface_cast<ICloneable>(other.result_)) {
        result_ = interface_pointer_cast<IAny>(p->GetClone());
    } else {
        result_.reset();
    }
    params_.clear();
    params_.resize(other.params_.size());
    for (int i = 0; i != params_.size(); ++i) {
        params_[i].name = other.params_[i].name;
        if (auto p = interface_cast<ICloneable>(other.params_[i].value)) {
            params_[i].value = interface_pointer_cast<IAny>(p->GetClone());
        }
    }
    return *this;
}

DefaultCallContext& DefaultCallContext::operator=(DefaultCallContext&& other) noexcept
{
    if (&other == this) {
        return *this;
    }
    result_ = std::move(other.result_);
    params_ = std::move(other.params_);
    return *this;
}

const CORE_NS::IInterface* DefaultCallContext::GetInterface(const BASE_NS::Uid& uid) const
{
    if (uid == CORE_NS::IInterface::UID || uid == ICallContext::UID) {
        return static_cast<const ICallContext*>(this);
    }
    if (uid == ICloneable::UID) {
        return static_cast<const ICloneable*>(this);
    }
    return nullptr;
}

CORE_NS::IInterface* DefaultCallContext::GetInterface(const BASE_NS::Uid& uid)
{
    const auto* me = this;
    return const_cast<CORE_NS::IInterface*>(me->DefaultCallContext::GetInterface(uid));
}

BASE_NS::shared_ptr<CORE_NS::IInterface> DefaultCallContext::GetClone() const
{
    BASE_NS::shared_ptr<DefaultCallContext> p(new DefaultCallContext(*this));
    return interface_pointer_cast<CORE_NS::IInterface>(p);
}

bool DefaultCallContext::DefineParameter(BASE_NS::string_view name, const IAny::Ptr& value)
{
    if (!name.empty() && Get(name)) {
        return false;
    }
    params_.push_back(ArgumentNameValue { BASE_NS::string(name), value });
    return true;
}

bool DefaultCallContext::Set(BASE_NS::string_view name, const IAny& value)
{
    for (auto&& v : params_) {
        if (v.name == name) {
            return v.value->CopyFrom(value);
        }
    }
    return false;
}

IAny::Ptr DefaultCallContext::Get(BASE_NS::string_view name) const
{
    if (!name.empty()) {
        for (auto&& v : params_) {
            if (v.name == name) {
                return v.value;
            }
        }
    }
    return nullptr;
}

BASE_NS::array_view<const ArgumentNameValue> DefaultCallContext::GetParameters() const
{
    return params_;
}

bool DefaultCallContext::Succeeded() const
{
    return succeeded_;
}

bool DefaultCallContext::DefineResult(const IAny::Ptr& value)
{
    result_ = value;
    return true;
}

bool DefaultCallContext::SetResult(const IAny& value)
{
    succeeded_ = result_ && result_->CopyFrom(value);
    if (!succeeded_) {
        ReportError("Invalid return type for meta function call");
    }
    return succeeded_;
}

bool DefaultCallContext::SetResult()
{
    // null for void, or otherwise return type
    succeeded_ = !result_;
    if (!succeeded_) {
        ReportError("Invalid return type for meta function call");
    }
    return succeeded_;
}

IAny::Ptr DefaultCallContext::GetResult() const
{
    return result_;
}

void DefaultCallContext::Reset()
{
    succeeded_ = false;
}

void DefaultCallContext::ReportError(BASE_NS::string_view error)
{
    // make sure it is null terminated
    CORE_LOG_W("Call context error: %s", BASE_NS::string(error).c_str());
}

META_END_NAMESPACE()
