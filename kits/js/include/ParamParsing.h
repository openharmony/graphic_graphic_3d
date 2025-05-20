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

#ifndef PARAM_PARSING_H
#define PARAM_PARSING_H

#include <napi_api.h>

#include <base/containers/string.h>

/**
 * @brief Extract node path from SceneNodeParameters args.
 * @param sceneNodeParameters a JS object that fulfills the SceneNodeParameters interface.
 * @return If the path member is set, return that. Otherwise return the name member.
 */
BASE_NS::string ExtractNodePath(NapiApi::Object sceneNodeParameters);

/**
 * @brief Extract name from SceneNodeParameters args.
 * @param sceneNodeParameters a JS object that fulfills the SceneNodeParameters interface.
 * @return The name member.
 */
BASE_NS::string ExtractName(NapiApi::Object sceneNodeParameters);

/**
 * @brief Extract URI from a Resource arg.
 * @param resource a JS object that fulfills the Resource interface.
 * @return The URI or an empty string if no URI is found.
 */
BASE_NS::string ExtractUri(NapiApi::FunctionContext<>& resource);

/**
 * @brief Extract URI from a Resource arg.
 * @param resource a JS object that fulfills the Resource interface.
 * @return The URI or an empty string if no URI is found.
 */
BASE_NS::string ExtractUri(NapiApi::Object resource);

/**
 * @brief Extract URI from a Resource arg.
 * @param resource a JS object that fulfills the Resource interface.
 * @return The URI or an empty string if no URI is found.
 */
BASE_NS::string ExtractUri(BASE_NS::string uri);

#endif
