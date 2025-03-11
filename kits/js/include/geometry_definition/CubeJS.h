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

#ifndef GEOMETRY_DEFINITION_CUBE_JS_H
#define GEOMETRY_DEFINITION_CUBE_JS_H

#include <meta/interface/intf_object.h>
#include <napi_api.h>

#include <base/math/vector.h>

#include "BaseObjectJS.h"
#include "geometry_definition/GeometryDefinition.h"

namespace GeometryDefinition {

class CubeJS : public GeometryDefinition<CubeJS> {
public:
    static constexpr uint32_t ID = 151;

    explicit CubeJS(napi_env, napi_callback_info);
    ~CubeJS() override = default;

    static void Init(napi_env env, napi_value exports);

    virtual void* GetInstanceImpl(uint32_t id) override;

    BASE_NS::Math::Vec3 GetSize() const;

private:
    napi_value GetSize(NapiApi::FunctionContext<>& ctx);
    void SetSize(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    BASE_NS::Math::Vec3 size_ {};
};

} // namespace GeometryDefinition

#endif
