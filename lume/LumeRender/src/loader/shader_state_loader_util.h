/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef LOADER_SHADER_STATE_LOADER_UTIL_H
#define LOADER_SHADER_STATE_LOADER_UTIL_H

#include <core/namespace.h>

#include "json_util.h"
#include "shader_state_loader.h"

RENDER_BEGIN_NAMESPACE()
/** Shader state loader util.
 * Functions that can be used to load shader (graphics) state data from a json.
 */
namespace ShaderStateLoaderUtil {
struct ShaderStateResult {
    ShaderStateLoader::LoadResult res;
    ShaderStateLoader::GraphicsStates states;
};
void ParseSingleState(const CORE_NS::json::value& jsonData, ShaderStateResult& ssr);
void ParseStateFlags(const CORE_NS::json::value& jsonData, GraphicsStateFlags& stateFlags, ShaderStateResult& ssr);
ShaderStateResult LoadStates(const CORE_NS::json::value& jsonData);
}; // namespace ShaderStateLoaderUtil
RENDER_END_NAMESPACE()

#endif // LOADER_SHADER_STATE_LOADER_UTIL_H
