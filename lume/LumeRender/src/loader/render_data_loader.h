/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
