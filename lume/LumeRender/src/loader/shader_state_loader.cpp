/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
        ShaderStateLoaderUtil::ShaderStateResult ssr;
        ssr.res.success = true;
        ssr.res.error = "Invalid json file.";
        return ssr;
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
