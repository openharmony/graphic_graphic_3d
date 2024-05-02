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

#ifndef OHOS_RENDER_3D_NODE_JS_H
#define OHOS_RENDER_3D_NODE_JS_H
#include <meta/interface/intf_object.h>
#include "BaseObjectJS.h"
#include "NodeImpl.h"

class NodeJS : public BaseObject<NodeJS>,NodeImpl {
public:
    static constexpr uint32_t ID = 130;
    static void Init(napi_env env, napi_value exports);
    NodeJS(napi_env,napi_callback_info);
    ~NodeJS()override;
    void* GetInstanceImpl(uint32_t id) override;

private:
    void DisposeNative() override;
};
#endif // OHOS_RENDER_3D_NODE_JS_H
