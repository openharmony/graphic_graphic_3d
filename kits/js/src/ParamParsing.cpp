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

#include "ParamParsing.h"

BASE_NS::string ExtractNodePath(NapiApi::Object params)
{
    const auto env = params.GetEnv();
    if (auto path = NapiApi::Value<BASE_NS::string> { env, params.Get("path") }; path.IsDefined()) {
        return path;
    }
    return ExtractName(params);
}

BASE_NS::string ExtractName(NapiApi::Object params)
{
    return NapiApi::Value<BASE_NS::string> { params.GetEnv(), params.Get("name") };
}

BASE_NS::string ExtractUri(NapiApi::FunctionContext<>& ctx)
{
    BASE_NS::string uri;
    NapiApi::FunctionContext<BASE_NS::string> uriStr(ctx);
    if (uriStr) {
        // actually not support anymore.
        uri = uriStr.Arg<0>();
        // check if there is a protocol
        auto t = uri.find("://");
        if (t == BASE_NS::string::npos)
        {
            // no proto . so use default
            uri.insert(0, "file://");
        }
        return uri;
    }
    // rawfile
    NapiApi::FunctionContext<NapiApi::Object> rawFileCtx(ctx);
    NapiApi::Object uriRawFile = rawFileCtx.Arg<0>();
    uint32_t id = uriRawFile.Get<uint32_t>("id");
    uint32_t resourceType = uriRawFile.Get<uint32_t>("type");
    NapiApi::Array parms = uriRawFile.Get<NapiApi::Array>("params");

    if ((id == 0) && (resourceType == 30000)) {  // 30000 : param
        // seems like a correct raw file.
        uri = parms.Get<BASE_NS::string>(0);
    }
    if (!uri.empty()) {
        // add the schema then
        uri.insert(0, "OhosRawFile://");
    }
    
    return uri;
}

BASE_NS::string ExtractUri(NapiApi::Object resource)
{
    uint32_t id = resource.Get<uint32_t>("id");
    uint32_t resourceType = resource.Get<uint32_t>("type");
    NapiApi::Array parms = resource.Get<NapiApi::Array>("params");
    BASE_NS::string uri;
    if ((id == 0) && (resourceType == 30000)) { // 30000: param
        // seems like a correct rawfile.
        uri = parms.Get<BASE_NS::string>(0);
    }
    if (!uri.empty()) {
        // add the schema then
        uri.insert(0, "OhosRawFile://");
        return uri;
    }
    auto e = resource.GetEnv();
    auto v = resource.ToNapiValue();
    napi_valuetype type;
    napi_typeof(e, v, &type);
    if (type == napi_string) {
        uri = NapiApi::Value<BASE_NS::string>(e, v);
        // check if there is a protocol
        auto t = uri.find("://");
        if (t == BASE_NS::string::npos) {
            // no proto . so use default
            // set system file as default format
            uri.insert(0, "file://");
        }
    }
    return uri;
}
