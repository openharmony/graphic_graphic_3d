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

#ifndef TEXTNODE_JS_H
#define TEXTNODE_JS_H

#include <meta/interface/intf_object.h>

#include "BaseObjectJS.h"
#include "NodeImpl.h"
#include "ColorProxy.h"

class TextNodeJS final : public BaseObject, NodeImpl {
public:
    static constexpr uint32_t ID = 140;
    static void Init(napi_env env, napi_value exports);
    TextNodeJS(napi_env, napi_callback_info);
    ~TextNodeJS() override;
    void* GetInstanceImpl(uint32_t) override;

private:
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;

private:
    napi_value GetText(NapiApi::FunctionContext<>& ctx);
    void SetText(NapiApi::FunctionContext<BASE_NS::string>& ctx);
    napi_value GetFont(NapiApi::FunctionContext<>& ctx);
    void SetFont(NapiApi::FunctionContext<BASE_NS::string>& ctx);
    napi_value GetColor(NapiApi::FunctionContext<>& ctx);
    void SetColor(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetFontSize(NapiApi::FunctionContext<>& ctx);
    void SetFontSize(NapiApi::FunctionContext<float>& ctx);

private:
    BASE_NS::unique_ptr<ColorProxy> color_;
};

#endif // TEXTNODE_JS_H
