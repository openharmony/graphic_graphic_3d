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

#ifndef OHOS_3D_RENDER_CONTEXT_IMPL_H
#define OHOS_3D_RENDER_CONTEXT_IMPL_H

#include "SceneTH.proj.hpp"
#include "SceneTH.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#include "RenderResourceFactoryImpl.h"

class RenderContextImpl {
public:
    RenderContextImpl()
    {
        // Don't forget to implement the constructor.
    }

    ::SceneTH::RenderResourceFactory getRenderResourceFactory()
    {
        // The parameters in the make_holder function should be of the same type
        // as the parameters in the constructor of the actual implementation class.
        return taihe::make_holder<RenderResourceFactoryImpl, ::SceneTH::RenderResourceFactory>();
    }

    bool loadPluginSync(::taihe::string_view name)
    {
        TH_THROW(std::runtime_error, "loadPluginSync not implemented");
    }
};
#endif  // OHOS_3D_RENDER_CONTEXT_IMPL_H