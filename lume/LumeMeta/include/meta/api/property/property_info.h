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

#ifndef META_API_PROPERTY_PROPERTY_INFO_H
#define META_API_PROPERTY_PROPERTY_INFO_H

#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_enum.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

/// Helper to access any IInfo for property
class PropertyInfo {
public:
    PropertyInfo(const IProperty::ConstPtr& p)
    {
        if (auto any = GetInternalAny(p)) {
            info_ = interface_pointer_cast<IInfo>(any);
        }
    }

    explicit operator bool() const
    {
        return info_ != nullptr;
    }

    const IInfo* operator->() const
    {
        return info_.get();
    }

private:
    IInfo::ConstPtr info_ {};
};

/// Helper to access constant enum info for property
class ConstPropertyEnumInfo {
public:
    ConstPropertyEnumInfo(const IProperty::Ptr& p)
    {
        if (auto any = GetInternalAny(p)) {
            info_ = interface_pointer_cast<IEnum>(any);
        }
    }

    explicit operator bool() const
    {
        return info_ != nullptr;
    }

    const IEnum* operator->() const
    {
        return info_.get();
    }

private:
    IEnum::ConstPtr info_ {};
};

class EnumInfoAccess {
public:
    EnumInfoAccess(IProperty* p, IAny* any) : p_(p), any_(any), lock_(p) {}

    ~EnumInfoAccess()
    {
        p_->SetValue(*any_);
    }

    IEnum* operator->()
    {
        return interface_cast<IEnum>(any_);
    }

private:
    IProperty* p_;
    IAny* any_;
    PropertyLock<IProperty> lock_;
};

/// Helper to access and change enum info for property
class PropertyEnumInfo {
public:
    PropertyEnumInfo(const IProperty::Ptr& p) : p_(p)
    {
        if (auto any = GetInternalAny(p)) {
            if (interface_cast<IEnum>(any)) {
                any_ = any->Clone();
            }
        }
    }

    explicit operator bool() const
    {
        return any_ != nullptr;
    }

    EnumInfoAccess operator->()
    {
        return EnumInfoAccess { p_.get(), any_.get() };
    }

private:
    IProperty::Ptr p_;
    IAny::Ptr any_;
};

META_END_NAMESPACE()

#endif
