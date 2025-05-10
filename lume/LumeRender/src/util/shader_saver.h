/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef RENDER_UTIL_SHADER_SAVER_H
#define RENDER_UTIL_SHADER_SAVER_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/json/json.h>
#include <core/namespace.h>
#include <render/device/intf_shader_manager.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()

IShaderManager::ShaderOutWriteResult SaveGraphicsState(const IShaderManager::ShaderGraphicsStateSaveInfo& saveInfo);
IShaderManager::ShaderOutWriteResult SaveVextexInputDeclarations(
    const IShaderManager::ShaderVertexInputDeclarationsSaveInfo& saveInfo);
IShaderManager::ShaderOutWriteResult SavePipelineLayouts(const IShaderManager::ShaderPipelineLayoutSaveInfo& saveInfo);
IShaderManager::ShaderOutWriteResult SaveVariants(const IShaderManager::ShaderVariantsSaveInfo& saveInfo);

RENDER_END_NAMESPACE()
#endif // RENDER_UTIL_SHADER_SAVER_H
