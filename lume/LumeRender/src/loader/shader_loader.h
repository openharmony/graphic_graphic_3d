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

#ifndef LOADER_SHADER_LOADER_H
#define LOADER_SHADER_LOADER_H

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/io/intf_directory.h>
#include <core/namespace.h>
#include <render/device/intf_device.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/shader_manager.h"
#include "loader/shader_data_loader.h"

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class PipelineLayoutLoader;
class VertexInputDeclarationLoader;
class ShaderStateLoader;
struct ShaderStateLoaderVariantData;

/** ShaderLoader.
 * A class that can be used to load all the shaders states from shaders:// and reload the shaders e.g. when the shader
 * has been modified. In addition handles vertex input declaration loading.
 */
class ShaderLoader {
public:
    /** Constructor.
     * @param fileManager File manager to be used when loading files.
     * @param aResourceManager Resource manager to be used when loading shaders.
     */
    ShaderLoader(CORE_NS::IFileManager& fileManager, ShaderManager& shaderManager, DeviceBackendType type);

    /** Destructor. */
    ~ShaderLoader() = default;

    /** Looks for json files with given paths, parses them, and loads the listed shaders. */
    void Load(const ShaderManager::ShaderFilePathDesc& desc);
    /** Looks for json files with given path, parses them, and loads the listed data. */
    void LoadFile(BASE_NS::string_view uri, const bool forceReload);

private:
    void HandleShaderFile(
        BASE_NS::string_view currentPath, const CORE_NS::IDirectory::Entry& entry, const bool forceReload);
    void HandleShaderStateFile(BASE_NS::string_view currentPath, const CORE_NS::IDirectory::Entry& entry);
    void HandlePipelineLayoutFile(BASE_NS::string_view currentPath, const CORE_NS::IDirectory::Entry& entry);
    void HandleVertexInputDeclarationFile(BASE_NS::string_view currentPath, const CORE_NS::IDirectory::Entry& entry);
    void RecurseDirectory(BASE_NS::string_view currentPath, const CORE_NS::IDirectory& directory);
    struct ShaderFile {
        BASE_NS::vector<uint8_t> data;
        BASE_NS::vector<uint8_t> reflectionData;
        ShaderModuleCreateInfo info;
    };
    ShaderFile LoadShaderFile(BASE_NS::string_view shader, ShaderStageFlags stageBits);
    RenderHandleReference CreateComputeShader(const ShaderDataLoader& dataLoader, const bool forceReload);
    RenderHandleReference CreateGraphicsShader(const ShaderDataLoader& dataLoader, const bool forceReload);
    RenderHandleReference CreateShader(const ShaderDataLoader& dataLoader, const bool forceReload);

    void LoadShaderStates(BASE_NS::string_view currentPath, const CORE_NS::IDirectory& directory);
    void CreateShaderStates(BASE_NS::string_view uri,
        const BASE_NS::array_view<const ShaderStateLoaderVariantData>& variantData,
        const BASE_NS::array_view<const GraphicsState>& states);

    void LoadVids(BASE_NS::string_view currentPath, const CORE_NS::IDirectory& directory);
    RenderHandleReference CreateVertexInputDeclaration(const VertexInputDeclarationLoader& loader);

    void LoadPipelineLayouts(BASE_NS::string_view currentPath, const CORE_NS::IDirectory& directory);
    RenderHandleReference CreatePipelineLayout(const PipelineLayoutLoader& loader);

    CORE_NS::IFileManager& fileManager_;
    ShaderManager& shaderMgr_;
    DeviceBackendType type_;

    struct ShaderModuleShaders {
        ShaderStageFlags shaderStageFlags { 0u };
        BASE_NS::vector<BASE_NS::string> shaderNames;
    };
#if (RENDER_DEV_ENABLED == 1)
    // Maps shader source file to shader resource names which use the shader.
    // For book-keeping in dev mode for spv reloading.
    BASE_NS::unordered_map<BASE_NS::string, ShaderModuleShaders> fileToShaderNames_;
#endif
};
RENDER_END_NAMESPACE()

#endif // LOADER_SHADER_LOADER_H
