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

#ifndef META_API_PROPERTY_ARRAY_ELEMENT_BIND_H
#define META_API_PROPERTY_ARRAY_ELEMENT_BIND_H

#include <meta/ext/event_impl.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

class ArrayElementBind : public IntroduceInterfaces<IValue, INotifyOnChange> {
public:
    ArrayElementBind(const IProperty::Ptr& p, size_t index) : p_(p), index_(index)
    {
        if (auto i = interface_cast<IPropertyInternalAny>(p)) {
            if (auto&& any = i->GetInternalAny()) {
                if (any->IsArray()) {
                    value_ = any->Clone({ CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM });
                }
            }
        }
        if (!value_) {
            CORE_LOG_E("Failed to set any for array element bind");
            value_ = GetObjectRegistry().GetPropertyRegister().InvalidAny().Clone(false);
        }
        p->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
            if (!settingValue_) {
                event_->Invoke();
            }
        }),
            (uintptr_t)this);
    }
    ~ArrayElementBind()
    {
        if (auto p = p_.lock()) {
            p->OnChanged()->RemoveHandler((uintptr_t)this);
        }
    }
    BASE_NS::shared_ptr<IEvent> EventOnChanged() const override
    {
        return event_;
    }
    AnyReturnValue SetValue(const IAny& value) override
    {
        if (auto p = p_.lock()) {
            AnyReturnValue ret = AnyReturn::FAIL;
            settingValue_ = true;
            {
                ArrayPropertyLock l(p);
                ret = l->SetAnyAt(index_, value);
            }
            settingValue_ = false;
            return ret;
        }
        return AnyReturn::FAIL;
    }
    const IAny& GetValue() const override
    {
        if (auto p = p_.lock()) {
            ArrayPropertyLock l(p);
            l->GetAnyAt(index_, *value_);
        }
        return *value_;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return META_NS::IsCompatible(*value_, id);
    }

private:
    std::atomic<bool> settingValue_ {};
    BASE_NS::shared_ptr<EventImpl<IOnChanged>> event_ { new EventImpl<IOnChanged>("OnChanged") };
    IProperty::WeakPtr p_;
    IAny::Ptr value_;
    std::size_t index_ {};
};

inline void AddArrayElementBind(const IProperty::Ptr& p, const IProperty::Ptr& arr, size_t index)
{
    if (auto i = interface_cast<IStackProperty>(p)) {
        BASE_NS::shared_ptr<ArrayElementBind> bind(new ArrayElementBind(arr, index));
        i->PushValue(interface_pointer_cast<IValue>(bind));
    }
}

META_END_NAMESPACE()

#endif