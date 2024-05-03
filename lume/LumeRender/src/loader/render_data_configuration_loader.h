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

#ifndef LOADER_RENDER_DATA_CONFIGURATION_LOADER_H
#define LOADER_RENDER_DATA_CONFIGURATION_LOADER_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/namespace.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/loader/intf_render_data_configuration_loader.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
/**
 * Render data configuration loader.
 * A class that can be used to load vertex input declaration from json structure.
 */
class RenderDataConfigurationLoader final {
public:
    /** Load PostProcessConfiguration.
     * @return A post process configuration, as defined in the json file.
     */
    static IRenderDataConfigurationLoader::LoadedPostProcess LoadPostProcess(BASE_NS::string_view jsonString);
    static IRenderDataConfigurationLoader::LoadedPostProcess LoadPostProcess(
        CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri);
};

class RenderDataConfigurationLoaderImpl final : public IRenderDataConfigurationLoader {
public:
    LoadedPostProcess LoadPostProcess(BASE_NS::string_view jsonString) override;
    LoadedPostProcess LoadPostProcess(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri) override;

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;
};
RENDER_END_NAMESPACE()

#endif // LOADER_RENDER_DATA_CONFIGURATION_LOADER_H
