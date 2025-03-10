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

 #ifndef MESH_RESOURCE_JS_H
 #define MESH_RESOURCE_JS_H
 
 #include <napi_api.h>
 
 #include "BaseObjectJS.h"
 #include "SceneResourceImpl.h"
 
 class MeshResourceJS : public BaseObject<MeshResourceJS>, public SceneResourceImpl {
 public:
     static constexpr uint32_t ID = 140;
 
     MeshResourceJS(napi_env, napi_callback_info);
     ~MeshResourceJS() override = default;
 
     static void Init(napi_env env, napi_value exports);
 
     virtual void* GetInstanceImpl(uint32_t id) override;
 
     NapiApi::StrongRef GetGeometryDefinition() const;
 
 private:
     void DisposeNative(void*) override;
     // This is a temporary solution. When IMeshResource is implemented, this can be removed.
     NapiApi::StrongRef geometryDefinition_ {};
 };
 
 #endif
 