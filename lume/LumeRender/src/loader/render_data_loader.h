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

#ifndef LOADER_RENDER_DATA_LOADER_H
#define LOADER_RENDER_DATA_LOADER_H

#include <base/containers/string_view.h>
#include <core/namespace.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
class IDirectory;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class IRenderDataStorePod;

/** RenderDataLoader.
 * A class that can be used to load all render data configuration from renderdataconfigurations://.
 */
class RenderDataLoader {
public:
    static constexpr char const* const RENDER_CONFIG_PATH = "renderdataconfigurations://";
    static constexpr char const* const POST_PROCESS_PATH = "postprocess";

    /** Constructor.
     * @param fileManager File manager to be used when loading files.
     */
    explicit RenderDataLoader(CORE_NS::IFileManager& fileManager);

    ~RenderDataLoader() = default;

    /** Looks for json files under pathPrefix + RENDER_CONFIG_PATH, parses them, and loads the files. */
    void Load(BASE_NS::string_view pathPrefix, IRenderDataStorePod& renderDataStorePod);

private:
    CORE_NS::IFileManager& fileManager_;

    enum class ConfigurationType : uint8_t {
        POST_PROCESS = 0,
    };

    void RecurseDirectory(BASE_NS::string_view currentPath, const CORE_NS::IDirectory& dir,
        const ConfigurationType configurationType, IRenderDataStorePod& renderDataStorePod);
};
RENDER_END_NAMESPACE()

#endif // LOADER_RENDER_DATA_LOADER_H
