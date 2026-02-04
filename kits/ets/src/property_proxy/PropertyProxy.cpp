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


#include "property_proxy/ColorProxy.h"
#include "property_proxy/EntityProxy.h"
#include "property_proxy/ImageProxy.h"
#include "property_proxy/QuatProxy.h"
#include "property_proxy/Vec2Proxy.h"
#include "property_proxy/Vec3Proxy.h"
#include "property_proxy/Vec4Proxy.h"

#include "property_proxy/PropertyProxy.h"

namespace OHOS::Render3D {
std::shared_ptr<IPropertyProxy> PropertyToProxy(const META_NS::IProperty::Ptr &prop)
{
    if (META_NS::IsCompatibleWith<float>(prop)) {
        return std::make_shared<PropertyProxy<float>>(prop);
    }
    if (META_NS::IsCompatibleWith<int32_t>(prop)) {
        return std::make_shared<PropertyProxy<int32_t>>(prop);
    }
    if (META_NS::IsCompatibleWith<uint32_t>(prop)) {
        return std::make_shared<PropertyProxy<uint32_t>>(prop);
    }
    if (META_NS::IsCompatibleWith<int64_t>(prop)) {
        return std::make_shared<PropertyProxy<int64_t>>(prop);
    }
    if (META_NS::IsCompatibleWith<uint64_t>(prop)) {
        return std::make_shared<PropertyProxy<uint64_t>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::string>(prop)) {
        return std::make_shared<PropertyProxy<BASE_NS::string>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec2>(prop)) {
        return std::make_shared<Vec2Proxy>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec3>(prop)) {
        return std::make_shared<Vec3Proxy>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Vec4>(prop)) {
        return std::make_shared<Vec4Proxy>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Math::Quat>(prop)) {
        return std::make_shared<QuatProxy>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::Color>(prop)) {
        return std::make_shared<ColorProxy>(prop);
    }
    if (META_NS::IsCompatibleWith<SCENE_NS::IImage::Ptr>(prop)) {
        return std::make_shared<ImageProxy>(prop);
    }
    // Not sure of the exact use case
    if (META_NS::IsCompatibleWith<CORE_NS::Entity>(prop)) {
        return std::make_shared<EntityProxy>(prop);
    }
    if (META_NS::IsCompatibleWith<bool>(prop)) {
        return std::make_shared<PropertyProxy<bool>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<BASE_NS::string>>(prop)) {
        return std::make_shared<ArrayPropertyProxy<BASE_NS::string>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<bool>>(prop)) {
        return std::make_shared<ArrayPropertyProxy<bool>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<int32_t>>(prop)) {
        return std::make_shared<ArrayPropertyProxy<int32_t>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<float>>(prop)) {
        return std::make_shared<ArrayPropertyProxy<float>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<BASE_NS::Math::Vec2>>(prop)) {
        return std::make_shared<ArrayPropertyProxy<BASE_NS::Math::Vec2>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<BASE_NS::Math::Vec3>>(prop)) {
        return std::make_shared<ArrayPropertyProxy<BASE_NS::Math::Vec3>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<BASE_NS::Math::Vec4>>(prop)) {
        return std::make_shared<ArrayPropertyProxy<BASE_NS::Math::Vec4>>(prop);
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<SCENE_NS::IImage::Ptr>>(prop)) {
        return std::make_shared<ArrayPropertyProxy<SCENE_NS::IImage::Ptr>>(prop);
    }
    auto any = META_NS::GetInternalAny(prop);
    CORE_LOG_F("Unsupported property type [%s]", any ? any->GetTypeIdString().c_str() : "<Unknown>");
    return nullptr;
}
}  // namespace OHOS::Render3D