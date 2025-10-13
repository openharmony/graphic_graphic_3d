/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_PROPERTY_PROXY_H
#define OHOS_3D_PROPERTY_PROXY_H

#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <base/util/color.h>
#include <meta/api/util.h>
#include <meta/interface/property/property.h>
#include <meta/interface/property/property_events.h>

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace OHOS::Render3D {
class IPropertyProxy {
public:
    virtual META_NS::IProperty::Ptr GetPropertyPtr() const = 0;
};

std::shared_ptr<IPropertyProxy> PropertyToProxy(const META_NS::IProperty::Ptr &prop);

template <typename Type>
class PropertyProxy : public IPropertyProxy {
public:
    explicit PropertyProxy(const META_NS::Property<Type> &prop) : prop_(prop.GetProperty())
    {
    }

    explicit PropertyProxy(const META_NS::IProperty::Ptr &prop) : prop_(prop)
    {
    }

    virtual ~PropertyProxy()
    {
        prop_.reset();
    }

    META_NS::IProperty::Ptr GetPropertyPtr() const override
    {
        return prop_;
    }

    void SetValue(const Type &value)
    {
        META_NS::Property<Type>(prop_)->SetValue(value);
    }

    Type GetValue() const
    {
        return META_NS::Property<Type>(prop_)->GetValue();
    }

protected:
    META_NS::IProperty::Ptr prop_;
};

template <typename Type>
class ArrayPropertyProxy : public IPropertyProxy {
public:
    explicit ArrayPropertyProxy(const META_NS::ArrayProperty<Type> &prop) : prop_(prop.GetProperty())
    {
    }

    explicit ArrayPropertyProxy(const META_NS::IProperty::Ptr &prop) : prop_(prop)
    {
    }

    virtual ~ArrayPropertyProxy()
    {
        prop_.reset();
    }

    META_NS::IProperty::Ptr GetPropertyPtr() const override
    {
        return prop_;
    }

    void SetValue(const BASE_NS::vector<Type> &value)
    {
        META_NS::ArrayProperty<Type>(prop_)->SetValue(value);
    }

    BASE_NS::vector<Type> GetValue() const
    {
        return META_NS::ArrayProperty<Type>(prop_)->GetValue();
    }

protected:
    META_NS::IProperty::Ptr prop_;
};

template <typename T>
std::string GetTypeName()
{
#ifdef __clang__
    std::string_view p = __PRETTY_FUNCTION__;
    size_t start = p.find("T = ") + 4;  // 4: length of "T = "
    size_t end = p.find("]", start);
    return std::string(p.substr(start, end - start));
#elif defined(__GNUC__)
    std::string_view p = __PRETTY_FUNCTION__;
    size_t start = p.find("T = ") + 4;  // 4: length of "T = "
    size_t end = p.find("]", start);
    return std::string(p.substr(start, end - start));
#elif defined(_MSC_VER)
    std::string_view p = __FUNCSIG__;
    size_t start = p.find("getTypeName<") + 12;  // 12: length of "getTypeName<"
    size_t end = p.find(">(void)", start);
    return std::string(p.substr(start, end - start));
#else
    return "<Unknown>";
#endif
}

template <typename Type>
void ProxySetProperty(std::shared_ptr<IPropertyProxy> proxy, const Type &value, const std::string &key)
{
    if (!proxy) {
        CORE_LOG_E("set proxy failed, proxy doesn't exist");
        return;
    }
    META_NS::IProperty::Ptr prop = proxy->GetPropertyPtr();
    if (!prop) {
        CORE_LOG_E("set prop failed, prop is invalid");
        return;
    }

    if (META_NS::IsCompatibleWith<Type>(prop)) {
        static_pointer_cast<PropertyProxy<Type>>(proxy)->SetValue(value);
        return;
    }
    if constexpr (std::is_same_v<Type, int32_t> || std::is_same_v<Type, float>) {
        if (META_NS::IsCompatibleWith<float>(prop)) {
            static_pointer_cast<PropertyProxy<float>>(proxy)->SetValue(static_cast<float>(value));
            CORE_LOG_W("property [%s] has type [float], but get [%s]", key.c_str(), GetTypeName<Type>().c_str());
            return;
        } else if (META_NS::IsCompatibleWith<int32_t>(prop)) {
            static_pointer_cast<PropertyProxy<int32_t>>(proxy)->SetValue(static_cast<int32_t>(value));
            CORE_LOG_W("property [%s] has type [int32_t], but get [%s]", key.c_str(), GetTypeName<Type>().c_str());
            return;
        } else if (META_NS::IsCompatibleWith<uint32_t>(prop)) {
            static_pointer_cast<PropertyProxy<uint32_t>>(proxy)->SetValue(
                static_cast<uint32_t>(static_cast<int32_t>(value)));
            CORE_LOG_W("property [%s] has type [uint32_t], but get [%s]", key.c_str(), GetTypeName<Type>().c_str());
            return;
        } else if (META_NS::IsCompatibleWith<int64_t>(prop)) {
            static_pointer_cast<PropertyProxy<int64_t>>(proxy)->SetValue(static_cast<int64_t>(value));
            CORE_LOG_W("property [%s] has type [int64_t], but get [%s]", key.c_str(), GetTypeName<Type>().c_str());
            return;
        } else if (META_NS::IsCompatibleWith<uint64_t>(prop)) {
            static_pointer_cast<PropertyProxy<uint64_t>>(proxy)->SetValue(
                static_cast<uint64_t>(static_cast<int64_t>(value)));
            CORE_LOG_W("property [%s] has type [uint64_t], but get [%s]", key.c_str(), GetTypeName<Type>().c_str());
            return;
        }
    }
    auto any = META_NS::GetInternalAny(prop);
    CORE_LOG_E("property [%s] has type [%s], but get [%s]", key.c_str(),
               any ? any->GetTypeIdString().c_str() : "<Unknown>", GetTypeName<Type>().c_str());
}

template <typename Type>
void ProxySetArrayProperty(std::shared_ptr<IPropertyProxy> proxy, const BASE_NS::vector<Type> &value,
                           const std::string &key)
{
    if (!proxy) {
        CORE_LOG_E("set proxy failed, proxy doesn't exist");
        return;
    }
    META_NS::IProperty::Ptr prop = proxy->GetPropertyPtr();
    if (!prop) {
        CORE_LOG_E("set prop failed, prop is invalid");
        return;
    }

    if (META_NS::IsCompatibleWith<BASE_NS::vector<Type>>(prop)) {
        static_pointer_cast<ArrayPropertyProxy<Type>>(proxy)->SetValue(value);
    } else {
        auto any = META_NS::GetInternalAny(prop);
        CORE_LOG_E("property [%s] has type [%s], but get [%s]", key.c_str(),
                   any ? any->GetTypeIdString().c_str() : "<Unknown>", GetTypeName<Type>().c_str());
    }
}
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_PROPERTY_PROXY_H