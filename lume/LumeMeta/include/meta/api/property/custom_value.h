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


#ifndef META_API_PROPERTY_CUSTOM_VALUE_H
#define META_API_PROPERTY_CUSTOM_VALUE_H

#include <meta/interface/interface_helpers.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

template<typename Type, typename GetFunc, typename SetFunc>
class CustomValue : public IntroduceInterfaces<IValue> {
public:
    CustomValue(GetFunc gfunc, SetFunc sfunc) : gfunc_(BASE_NS::move(gfunc)), sfunc_(BASE_NS::move(sfunc)) {}

    AnyReturnValue SetValue(const IAny& any) override
    {
        Type value {};
        auto res = any.GetValue(value);
        if (res) {
            if (sfunc_(value)) {
                res = any_.SetValue(value);
            } else {
                res = AnyReturn::FAIL;
            }
        }
        return res;
    }
    const IAny& GetValue() const override
    {
        auto value = gfunc_();
        if (any_.SetValue(value)) {
            return any_;
        }
        return GetObjectRegistry().GetPropertyRegister().InvalidAny();
    }

    bool IsCompatible(const TypeId& id) const override
    {
        return any_.IsCompatible(id);
    }

private:
    GetFunc gfunc_;
    SetFunc sfunc_;
    mutable Any<Type> any_;
};

template<typename Type, typename GetFunc, typename SetFunc>
bool AddCustomValue(const IProperty::Ptr& p, GetFunc gf, SetFunc sf)
{
    if (auto i = interface_cast<IStackProperty>(p)) {
        auto val = CreateShared<CustomValue<Type, GetFunc, SetFunc>>(BASE_NS::move(gf), BASE_NS::move(sf));
        return i->PushValue(val);
    }
    return false;
}

template<typename Type, typename Obj>
bool AddGetSetCustomValue(const Property<Type>& p, Obj* obj, Type (Obj::*getf)() const, bool (Obj::*setf)(const Type&))
{
    IStackProperty::Ptr sp = interface_pointer_cast<IStackProperty>(p);
    if (!sp) {
        return false;
    }
    auto gf = [obj, getf, weak = BASE_NS::weak_ptr<IObject>(obj->GetSelf())]() -> Type {
        if (auto lock = weak.lock()) {
            return (obj->*getf)();
        }
        return Type {};
    };
    auto sf = [obj, setf, weak = BASE_NS::weak_ptr<IObject>(obj->GetSelf())](const Type& value) -> bool {
        if (auto lock = weak.lock()) {
            return (obj->*setf)(value);
        }
        return false;
    };
    return AddCustomValue<Type>(p.GetProperty(), BASE_NS::move(gf), BASE_NS::move(sf));
}

META_END_NAMESPACE()

#endif
