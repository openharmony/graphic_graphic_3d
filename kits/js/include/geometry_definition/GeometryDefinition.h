/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef GEOMETRY_DEFINITION_GEOMETRY_DEFINITION_H
#define GEOMETRY_DEFINITION_GEOMETRY_DEFINITION_H

#include <meta/interface/intf_object.h>
#include <napi_api.h>

#include <base/containers/vector.h>

#include "BaseObjectJS.h"

namespace GeometryDefinition {

enum GeometryType {
    CUSTOM = 0,
    CUBE = 1,
    PLANE = 2,
    SPHERE = 3,
    INVALID = 4,
};
void RegisterEnums(NapiApi::Object exports);

template<class FinalType>
class GeometryDefinition : public BaseObject<FinalType> {
public:
    GeometryDefinition(napi_env env, napi_callback_info info, GeometryType type)
        : BaseObject<FinalType>(env, info), type_(type)
    {}
    virtual ~GeometryDefinition() override = default;

    static void GetPropertyDescs(BASE_NS::vector<napi_property_descriptor>& props)
    {
        using namespace NapiApi;
        props.push_back(GetProperty<uint32_t, GeometryDefinition, &GeometryDefinition::GetType>("geometryType"));
    }

    void SetNativeObject(META_NS::IObject::Ptr real, bool Strong) override
    {
        LOG_E("GeometryDefinitions aren't meant to have a native object");
    }

private:
    napi_value GetType(NapiApi::FunctionContext<>& ctx)
    {
        return ctx.GetNumber(type_);
    }

    virtual void DisposeNative(void*) override
    {
        this->disposed_ = true;
    }

    GeometryType type_ { INVALID };
};

} // namespace GeometryDefinition

#endif
