/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_LOADER_IRENDER_DATA_CONFIGURATION_LOADER_H
#define API_RENDER_LOADER_IRENDER_DATA_CONFIGURATION_LOADER_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
/**
 * Render data configuration loader.
 * A class that can be used to load vertex input declaration from json structure.
 */
class IRenderDataConfigurationLoader : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "dca91e7e-1b03-47a8-a09e-2dbd99acaa9d" };

    /** Describes result of the parsing operation. */
    struct LoadResult {
        LoadResult() = default;
        explicit LoadResult(BASE_NS::string_view error) : success(false), error(error) {}

        /** Indicates, whether the parsing operation is successful. */
        bool success { true };

        /** In case of parsing error, contains the description of the error. */
        BASE_NS::string error;
    };

    struct LoadedPostProcess {
        LoadResult loadResult;

        PostProcessConfiguration postProcessConfiguration;
        BASE_NS::string name;
    };

    /** Load PostProcessConfiguration.
     * @return A post process configuration, as defined in the json file.
     */
    virtual LoadedPostProcess LoadPostProcess(BASE_NS::string_view jsonString) = 0;
    virtual LoadedPostProcess LoadPostProcess(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri) = 0;

protected:
    IRenderDataConfigurationLoader() = default;
    virtual ~IRenderDataConfigurationLoader() = default;
};

inline constexpr const BASE_NS::string_view GetName(const IRenderDataConfigurationLoader*)
{
    return "IRenderDataConfigurationLoader";
}
RENDER_END_NAMESPACE()

#endif // LOADER_RENDER_DATA_CONFIGURATION_LOADER_H
