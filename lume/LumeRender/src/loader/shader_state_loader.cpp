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

#include "shader_state_loader.h"

#include <cctype>
#include <locale>

#include <base/containers/string.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>

#include "json_util.h"
#include "shader_state_loader_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
ShaderStateLoaderUtil::ShaderStateResult LoadImpl(const string_view jsonString)
{
    const auto json = json::parse(jsonString.data());
    if (json) {
        ShaderStateLoaderUtil::ShaderStateResult ssr = ShaderStateLoaderUtil::LoadStates(json);
        return ssr;
    } else {
        return ShaderStateLoaderUtil::ShaderStateResult { ShaderStateLoader::LoadResult { "Invalid json file." }, {} };
    }
}
} // namespace

ShaderStateLoader::LoadResult ShaderStateLoader::Load(IFileManager& fileManager, const string_view uri)
{
    uri_ = uri;
    IFile::Ptr file = fileManager.OpenFile(uri);
    if (!file) {
        PLUGIN_LOG_D("Error loading '%s'", uri_.c_str());
        return LoadResult("Failed to open file.");
    }

    const uint64_t byteLength = file->GetLength();

    string raw;
    raw.resize(static_cast<size_t>(byteLength));

    if (file->Read(raw.data(), byteLength) != byteLength) {
        PLUGIN_LOG_D("Error loading '%s'", uri_.c_str());
        return LoadResult("Failed to read file.");
    }

    ShaderStateLoaderUtil::ShaderStateResult ssr = RENDER_NS::LoadImpl(string_view(raw));
    graphicsStates_ = move(ssr.states.states);
    graphicsStateVariantData_ = move(ssr.states.variantData);

    return ssr.res;
}

string_view ShaderStateLoader::GetUri() const
{
    return uri_;
}

array_view<const ShaderStateLoaderVariantData> ShaderStateLoader::GetGraphicsStateVariantData() const
{
    return graphicsStateVariantData_;
}

array_view<const GraphicsState> ShaderStateLoader::GetGraphicsStates() const
{
    return graphicsStates_;
}

RENDER_END_NAMESPACE()
