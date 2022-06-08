/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
ShaderStateResult LoadStates(const CORE_NS::json::value& jsonData);
}; // namespace ShaderStateLoaderUtil
RENDER_END_NAMESPACE()

#endif // LOADER_SHADER_STATE_LOADER_UTIL_H
