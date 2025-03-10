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

#ifndef GEOMETRY_DEFINITION_SPHERE_JS_H
#define GEOMETRY_DEFINITION_SPHERE_JS_H

#include <meta/interface/intf_object.h>
#include <napi_api.h>

#include <base/math/vector.h>

#include "BaseObjectJS.h"
#include "geometry_definition/GeometryDefinition.h"

namespace GeometryDefinition {

class SphereJS : public GeometryDefinition<SphereJS> {
public:
    static constexpr uint32_t ID = 153;

    explicit SphereJS(napi_env, napi_callback_info);
    ~SphereJS() override = default;

    static void Init(napi_env env, napi_value exports);

    virtual void* GetInstanceImpl(uint32_t id) override;

    float GetRadius() const;
    uint32_t GetSegmentCount() const;

private:
    napi_value GetRadius(NapiApi::FunctionContext<>& ctx);
    napi_value GetSegmentCount(NapiApi::FunctionContext<>& ctx);
    void SetRadius(NapiApi::FunctionContext<float>& ctx);
    void SetSegmentCount(NapiApi::FunctionContext<uint32_t>& ctx);
    float radius_ { 0 };
    uint32_t segmentCount_ { 0 };
};

} // namespace GeometryDefinition

#endif
