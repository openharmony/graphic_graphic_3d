/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SCENE_INTERFACE_SHADER_UTIL_H
#define SCENE_INTERFACE_SHADER_UTIL_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>
#include <scene/interface/intf_shader.h>

#include <meta/base/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IShaderUtil : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IShaderUtil, "bc7f1cd1-c70c-47a6-8deb-8f8772d9b711")
public:
    virtual Future<IShader::Ptr> CreateDefaultShader() const = 0;
};

META_REGISTER_CLASS(ShaderUtil, "bf3bfbfa-1aff-48bf-b6a1-4943caf7f9fb", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IShaderUtil)

#endif
