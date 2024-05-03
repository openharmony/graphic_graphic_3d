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

#include "loader/render_data_loader.h"

#include <base/containers/string_view.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store_pod.h>

#include "loader/render_data_configuration_loader.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
bool HasExtension(string_view ext, const string_view fileUri)
{
    if (auto const pos = fileUri.rfind(ext); pos != string_view::npos) {
        return ext.size() == (fileUri.length() - pos);
    }
    return false;
}
} // namespace

RenderDataLoader::RenderDataLoader(IFileManager& fileManager) : fileManager_(fileManager) {}

void RenderDataLoader::Load(const BASE_NS::string_view pathPrefix, IRenderDataStorePod& renderDataStorePod)
{
    const string fullPath = pathPrefix + RENDER_CONFIG_PATH;
    if (auto const path = fileManager_.OpenDirectory(fullPath); path) {
        const string currentPath = fullPath + string_view(POST_PROCESS_PATH) + '/';
        auto currentDirectory = fileManager_.OpenDirectory(currentPath);
        if (currentDirectory) {
            RecurseDirectory(currentPath, *currentDirectory, ConfigurationType::POST_PROCESS, renderDataStorePod);
        } else {
            PLUGIN_LOG_E("render configuration files path (%s) not found.", currentPath.c_str());
        }
    } else {
        PLUGIN_LOG_E("render configuration files path (%s) not found.", fullPath.c_str());
    }
}

void RenderDataLoader::RecurseDirectory(const string_view currentPath, const IDirectory& directory,
    const ConfigurationType configurationType, IRenderDataStorePod& renderDataStorePod)
{
    for (auto const& entry : directory.GetEntries()) {
        switch (entry.type) {
            default:
            case IDirectory::Entry::Type::UNKNOWN:
                break;
            case IDirectory::Entry::Type::FILE: {
                if (HasExtension(".json", entry.name)) {
                    auto const file = currentPath + entry.name;
                    IRenderDataConfigurationLoader::LoadResult result;
                    if (configurationType == ConfigurationType::POST_PROCESS) {
                        const auto loadedPP = RenderDataConfigurationLoader::LoadPostProcess(fileManager_, file);
                        if (loadedPP.loadResult.success) {
                            renderDataStorePod.CreatePod(
                                "PostProcess", loadedPP.name, arrayviewU8(loadedPP.postProcessConfiguration));
                        }
                        result = loadedPP.loadResult;
                    }

                    if (!result.success) {
                        PLUGIN_LOG_E("unable to load renderer data configuration json %s : %s", file.c_str(),
                            result.error.c_str());
                    }
                }
            } break;
            case IDirectory::Entry::Type::DIRECTORY: {
                PLUGIN_LOG_I("recursive renderer configuration directories not supported");
            } break;
        }
    }
}
RENDER_END_NAMESPACE()
