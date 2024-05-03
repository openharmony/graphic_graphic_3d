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

#include "loader/shader_loader.h"

#include <cstring>
#include <set>

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <core/io/intf_file_manager.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

#include "device/shader_manager.h"
#include "loader/json_util.h"
#include "loader/pipeline_layout_loader.h"
#include "loader/shader_data_loader.h"
#include "loader/shader_state_loader.h"
#include "loader/vertex_input_declaration_loader.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
enum ShaderDataFileType : uint32_t {
    UNDEFINED = 0,
    SHADER = 1,
    SHADER_STATE = 2,
    VERTEX_INPUT_DECLARATION = 3,
    PIPELINE_LAYOUT = 4,
};

constexpr string_view ShaderDataFileExtensions[] {
    "",
    ".shader",
    ".shadergs",
    ".shadervid",
    ".shaderpl",
};

bool HasExtension(const char* ext, const string_view fileUri)
{
    if (auto const pos = fileUri.rfind(ext); pos != string_view::npos) {
        return std::strlen(ext) == (fileUri.length() - pos);
    }
    return false;
}

ShaderDataFileType GetShaderDataFileType(IFileManager& fileMgr, const string_view fullFilename)
{
    if (HasExtension(ShaderDataFileExtensions[ShaderDataFileType::SHADER].data(), fullFilename)) {
        return ShaderDataFileType::SHADER;
    } else if (HasExtension(ShaderDataFileExtensions[ShaderDataFileType::SHADER_STATE].data(), fullFilename)) {
        return ShaderDataFileType::SHADER_STATE;
    } else if (HasExtension(ShaderDataFileExtensions[ShaderDataFileType::PIPELINE_LAYOUT].data(), fullFilename)) {
        return ShaderDataFileType::PIPELINE_LAYOUT;
    } else if (HasExtension(
                   ShaderDataFileExtensions[ShaderDataFileType::VERTEX_INPUT_DECLARATION].data(), fullFilename)) {
        return ShaderDataFileType::VERTEX_INPUT_DECLARATION;
    }
    return ShaderDataFileType::UNDEFINED;
}

vector<uint8_t> ReadFile(IFile& file)
{
    auto fileData = vector<uint8_t>(static_cast<std::size_t>(file.GetLength()));
    file.Read(fileData.data(), fileData.size());
    return fileData;
}
} // namespace

ShaderLoader::ShaderLoader(IFileManager& fileManager, ShaderManager& shaderManager, const DeviceBackendType type)
    : fileManager_(fileManager), shaderMgr_(shaderManager), type_(type)
{}

void ShaderLoader::Load(const ShaderManager::ShaderFilePathDesc& desc)
{
    if (!desc.shaderStatePath.empty()) {
        auto const shaderStatesPath = fileManager_.OpenDirectory(desc.shaderStatePath);
        if (shaderStatesPath) {
            LoadShaderStates(desc.shaderStatePath, *shaderStatesPath);
        } else {
            PLUGIN_LOG_W("graphics state path (%s) not found.", desc.shaderStatePath.data());
        }
    }
    if (!desc.vertexInputDeclarationPath.empty()) {
        auto const vidsPath = fileManager_.OpenDirectory(desc.vertexInputDeclarationPath);
        if (vidsPath) {
            LoadVids(desc.vertexInputDeclarationPath, *vidsPath);
        } else {
            PLUGIN_LOG_W("vertex input declaration path (%s) not found.", desc.vertexInputDeclarationPath.data());
        }
    }
    if (!desc.pipelineLayoutPath.empty()) {
        auto const pipelineLayoutsPath = fileManager_.OpenDirectory(desc.pipelineLayoutPath);
        if (pipelineLayoutsPath) {
            LoadPipelineLayouts(desc.pipelineLayoutPath, *pipelineLayoutsPath);
        } else {
            PLUGIN_LOG_W("pipeline layout path (%s) not found.", desc.pipelineLayoutPath.data());
        }
    }
    if (!desc.shaderPath.empty()) {
        auto const shadersPath = fileManager_.OpenDirectory(desc.shaderPath);
        if (shadersPath) {
            RecurseDirectory(desc.shaderPath, *shadersPath);
        } else {
            PLUGIN_LOG_W("shader path (%s) not found.", desc.shaderPath.data());
        }
    }
}

void ShaderLoader::LoadFile(const string_view uri, const bool forceReload)
{
    const IDirectory::Entry entry = fileManager_.GetEntry(uri);
    if (entry.type == IDirectory::Entry::FILE) {
        // NOTE: currently there's no info within the shader json files
        // we do type evaluation based on some key names in the json
        const ShaderDataFileType shaderDataFileType = GetShaderDataFileType(fileManager_, uri);
        switch (shaderDataFileType) {
            case ShaderDataFileType::SHADER: {
                // Force re-loads the shader module creation
                HandleShaderFile(uri, entry, forceReload);
                break;
            }
            case ShaderDataFileType::SHADER_STATE: {
                HandleShaderStateFile(uri, entry);
                break;
            }
            case ShaderDataFileType::PIPELINE_LAYOUT: {
                HandlePipelineLayoutFile(uri, entry);
                break;
            }
            case ShaderDataFileType::VERTEX_INPUT_DECLARATION: {
                HandleVertexInputDeclarationFile(uri, entry);
                break;
            }
            default: {
                break;
            }
        }
    }
}

void ShaderLoader::HandleShaderFile(
    const string_view fullFileName, const IDirectory::Entry& entry, const bool forceReload)
{
    if (HasExtension(ShaderDataFileExtensions[ShaderDataFileType::SHADER].data(), entry.name)) {
        ShaderDataLoader loader;
        const auto result = loader.Load(fileManager_, fullFileName);
        if (result.success) {
            auto const handle = CreateShader(loader, forceReload);
#if (RENDER_DEV_ENABLED == 1)
            const auto shaderVariants = loader.GetShaderVariants();
            for (const auto& shaderVariant : shaderVariants) {
                // Dev related book-keeping for reloading of spv files
                auto const handleType = handle.GetHandleType();
                if (handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
                    string compShader = shaderVariant.computeShader;
                    PLUGIN_ASSERT(!compShader.empty());

                    auto& ref = fileToShaderNames_[move(compShader)];
                    ref.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT;
                    ref.shaderNames.emplace_back(fullFileName);
                } else if (handleType == RenderHandleType::SHADER_STATE_OBJECT) {
                    string vertShader = shaderVariant.vertexShader;
                    string fragShader = shaderVariant.fragmentShader;
                    PLUGIN_ASSERT_MSG(
                        (!vertShader.empty()) && (!fragShader.empty()), "shader name: %s", fullFileName.data());

                    auto& refVert = fileToShaderNames_[move(vertShader)];
                    refVert.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT;
                    refVert.shaderNames.emplace_back(fullFileName);
                    auto& refFrag = fileToShaderNames_[move(fragShader)];
                    refFrag.shaderStageFlags = ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT;
                    refFrag.shaderNames.emplace_back(fullFileName);
                }
            }
#endif
        } else {
            PLUGIN_LOG_E("unable to load shader json %s : %s", fullFileName.data(), result.error.c_str());
        }
    }
}

void ShaderLoader::HandleShaderStateFile(const string_view fullFileName, const IDirectory::Entry& entry)
{
    if (HasExtension(ShaderDataFileExtensions[ShaderDataFileType::SHADER_STATE].data(), entry.name)) {
        ShaderStateLoader loader;
        const auto result = loader.Load(fileManager_, fullFileName);
        if (result.success) {
            CreateShaderStates(loader.GetUri(), loader.GetGraphicsStateVariantData(), loader.GetGraphicsStates());
        } else {
            PLUGIN_LOG_E("unable to load shader state json %s : %s", fullFileName.data(), result.error.c_str());
        }
    }
}

void ShaderLoader::HandlePipelineLayoutFile(const string_view fullFileName, const IDirectory::Entry& entry)
{
    if (HasExtension(ShaderDataFileExtensions[ShaderDataFileType::PIPELINE_LAYOUT].data(), entry.name)) {
        PipelineLayoutLoader loader;
        const auto result = loader.Load(fileManager_, fullFileName);
        if (result.success) {
            auto const handle = CreatePipelineLayout(loader);
            if (!handle) {
                PLUGIN_LOG_E(
                    "pipeline layout could not be created (%s) (%s)", fullFileName.data(), loader.GetUri().data());
            }
        } else {
            PLUGIN_LOG_E("unable to load pipeline layout json %s : %s", fullFileName.data(), result.error.c_str());
        }
    }
}

void ShaderLoader::HandleVertexInputDeclarationFile(const string_view fullFileName, const IDirectory::Entry& entry)
{
    if (HasExtension(ShaderDataFileExtensions[ShaderDataFileType::VERTEX_INPUT_DECLARATION].data(), entry.name)) {
        VertexInputDeclarationLoader loader;
        const auto result = loader.Load(fileManager_, fullFileName);
        if (result.success) {
            auto const vidName = loader.GetUri();
            auto const handle = CreateVertexInputDeclaration(loader);
            if (!handle) {
                PLUGIN_LOG_E(
                    "vertex input declaration could not be created (%s) (%s)", fullFileName.data(), vidName.data());
            }
        } else {
            PLUGIN_LOG_E(
                "unable to load vertex input declaration json %s : %s", fullFileName.data(), result.error.c_str());
        }
    }
}

void ShaderLoader::RecurseDirectory(const string_view currentPath, const IDirectory& directory)
{
    for (auto const& entry : directory.GetEntries()) {
        switch (entry.type) {
            default:
            case IDirectory::Entry::Type::UNKNOWN:
                break;
            case IDirectory::Entry::Type::FILE: {
                // does not force the shader module re-creations
                HandleShaderFile(currentPath + entry.name, entry, false);
                break;
            }
            case IDirectory::Entry::Type::DIRECTORY: {
                if (entry.name == "." || entry.name == "..") {
                    continue;
                }
                auto nextDirectory = currentPath + entry.name + '/';
                auto dir = fileManager_.OpenDirectory(nextDirectory);
                if (dir) {
                    RecurseDirectory(nextDirectory, *dir);
                }
                break;
            }
        }
    }
}

ShaderLoader::ShaderFile ShaderLoader::LoadShaderFile(const string_view shader, const ShaderStageFlags stageBits)
{
    ShaderLoader::ShaderFile info;
    IFile::Ptr shaderFile;
    switch (type_) {
        case DeviceBackendType::VULKAN:
            shaderFile = fileManager_.OpenFile(shader);
            break;
        case DeviceBackendType::OPENGLES:
            shaderFile = fileManager_.OpenFile(shader + ".gles");
            break;
        case DeviceBackendType::OPENGL:
            shaderFile = fileManager_.OpenFile(shader + ".gl");
            break;
        default:
            break;
    }
    if (shaderFile) {
        info.data = ReadFile(*shaderFile);

        if (IFile::Ptr reflectionFile = fileManager_.OpenFile(shader + ".lsb"); reflectionFile) {
            info.reflectionData = ReadFile(*reflectionFile);
        }
        info.info = { stageBits, info.data, { info.reflectionData } };
    } else {
        PLUGIN_LOG_E("shader file not found (%s)", shader.data());
    }
    return info;
}

RenderHandleReference ShaderLoader::CreateComputeShader(const ShaderDataLoader& dataLoader, const bool forceReload)
{
    RenderHandleReference firstShaderVariantRhr;
    const array_view<const ShaderDataLoader::ShaderVariant> shaderVariants = dataLoader.GetShaderVariants();
    for (const auto& shaderVariant : shaderVariants) {
        const string_view computeShader = shaderVariant.computeShader;
        uint32_t index = INVALID_SM_INDEX;
        if (!forceReload) {
            index = shaderMgr_.GetShaderModuleIndex(computeShader);
        }
        if (index == INVALID_SM_INDEX) {
            const auto shaderFile = LoadShaderFile(computeShader, ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT);
            if (!shaderFile.data.empty()) {
                index = shaderMgr_.CreateShaderModule(computeShader, shaderFile.info);
            } else {
                PLUGIN_LOG_E("shader file not found (%s)", computeShader.data());
            }
        }
        if (index != INVALID_SM_INDEX) {
            const string_view uri = dataLoader.GetUri();
            const string_view baseShaderPath = dataLoader.GetBaseShader();
            const string_view baseCategory = dataLoader.GetBaseCategory();
            const string_view variantName = shaderVariant.variantName;
            const string_view displayName = shaderVariant.displayName;
            const string_view pipelineLayout = shaderVariant.pipelineLayout;
            const string_view renderSlot = shaderVariant.renderSlot;
            const string_view shaderFileStr = shaderVariant.shaderFileStr;
            const string_view matMetadataStr = shaderVariant.materialMetadata;
            const uint32_t rsId = shaderMgr_.CreateRenderSlotId(renderSlot);
            const uint32_t catId = shaderMgr_.CreateCategoryId(baseCategory);
            const uint32_t plIndex =
                (pipelineLayout.empty())
                    ? INVALID_SM_INDEX
                    : RenderHandleUtil::GetIndexPart(shaderMgr_.GetPipelineLayoutHandle(pipelineLayout).GetHandle());
            // if many variants, the first is shader created without variant name
            // it will have additional name for searching though
            RenderHandleReference rhr;
            if (!firstShaderVariantRhr) {
                // NOTE: empty variant name
                rhr = shaderMgr_.Create(
                    ComputeShaderCreateData { uri, rsId, catId, plIndex, index, shaderFileStr, matMetadataStr },
                    { baseShaderPath, {}, displayName });
                firstShaderVariantRhr = rhr;
                // add additional fullname with variant for the base shader
                if (!variantName.empty()) {
                    shaderMgr_.AddAdditionalNameForHandle(rhr, uri);
                }
            } else {
                rhr = shaderMgr_.Create(
                    ComputeShaderCreateData { uri, rsId, catId, plIndex, index, shaderFileStr, matMetadataStr },
                    { baseShaderPath, variantName, displayName });
            }
            if (shaderVariant.renderSlotDefaultShader) {
                shaderMgr_.SetRenderSlotData(rsId, rhr, {});
            }
        } else {
            PLUGIN_LOG_E("Failed to load shader : %s", computeShader.data());
        }
    }
    return firstShaderVariantRhr;
}

RenderHandleReference ShaderLoader::CreateGraphicsShader(const ShaderDataLoader& dataLoader, const bool forceReload)
{
    RenderHandleReference firstShaderVariantRhr;
    const array_view<const ShaderDataLoader::ShaderVariant> shaderVariants = dataLoader.GetShaderVariants();
    for (const auto& svRef : shaderVariants) {
        const string_view vertexShader = svRef.vertexShader;
        const string_view fragmentShader = svRef.fragmentShader;
        uint32_t vertIndex = (forceReload) ? INVALID_SM_INDEX : shaderMgr_.GetShaderModuleIndex(vertexShader);
        if (vertIndex == INVALID_SM_INDEX) {
            const auto shaderFile = LoadShaderFile(vertexShader, ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT);
            if (!shaderFile.data.empty()) {
                vertIndex = shaderMgr_.CreateShaderModule(vertexShader, shaderFile.info);
            }
        }
        uint32_t fragIndex = (forceReload) ? INVALID_SM_INDEX : shaderMgr_.GetShaderModuleIndex(fragmentShader);
        if (fragIndex == INVALID_SM_INDEX) {
            const auto shaderFile = LoadShaderFile(fragmentShader, ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT);
            if (!shaderFile.data.empty()) {
                fragIndex = shaderMgr_.CreateShaderModule(fragmentShader, shaderFile.info);
            }
        }
        if ((vertIndex != INVALID_SM_INDEX) && (fragIndex != INVALID_SM_INDEX)) {
            const string_view uri = dataLoader.GetUri();
            // creating the default graphics state with full name
            const string fullName = uri + svRef.variantName;
            // default graphics state is created beforehand
            const RenderHandleReference defaultGfxState = shaderMgr_.CreateGraphicsState(
                { fullName, svRef.graphicsState }, { svRef.renderSlot, {}, {}, {}, svRef.stateFlags });
            const uint32_t rsId = shaderMgr_.CreateRenderSlotId(svRef.renderSlot);
            const uint32_t catId = shaderMgr_.CreateCategoryId(dataLoader.GetBaseCategory());
            const uint32_t plIndex = svRef.pipelineLayout.empty()
                                         ? INVALID_SM_INDEX
                                         : RenderHandleUtil::GetIndexPart(
                                            shaderMgr_.GetPipelineLayoutHandle(svRef.pipelineLayout).GetHandle());
            const uint32_t vidIndex =
                svRef.vertexInputDeclaration.empty()
                    ? INVALID_SM_INDEX
                    : RenderHandleUtil::GetIndexPart(
                          shaderMgr_.GetVertexInputDeclarationHandle(svRef.vertexInputDeclaration).GetHandle());
            const uint32_t stateIndex = RenderHandleUtil::GetIndexPart(defaultGfxState.GetHandle());
            const string_view shaderStr = svRef.shaderFileStr;
            const string_view matMeta = svRef.materialMetadata;
            // if many variants, the first is shader created without variant name
            // it will have additional name for searching though
            RenderHandleReference rhr;
            if (!firstShaderVariantRhr) {
                // NOTE: empty variant name
                rhr = shaderMgr_.Create(
                    { uri, rsId, catId, vidIndex, plIndex, stateIndex, vertIndex, fragIndex, shaderStr, matMeta },
                    { dataLoader.GetBaseShader(), {}, svRef.displayName });
                firstShaderVariantRhr = rhr;
                // add additional fullname with variant for the base shader
                if (!svRef.variantName.empty()) {
                    shaderMgr_.AddAdditionalNameForHandle(firstShaderVariantRhr, fullName);
                }
            } else {
                rhr = shaderMgr_.Create(
                    { uri, rsId, catId, vidIndex, plIndex, stateIndex, vertIndex, fragIndex, shaderStr, matMeta },
                    { dataLoader.GetBaseShader(), svRef.variantName, svRef.displayName });
            }
            if (svRef.renderSlotDefaultShader) {
                shaderMgr_.SetRenderSlotData(rsId, rhr, {});
            }
        } else {
            PLUGIN_LOG_E("Failed to load shader : %s %s", vertexShader.data(), fragmentShader.data());
        }
    }
    return firstShaderVariantRhr;
}

RenderHandleReference ShaderLoader::CreateShader(const ShaderDataLoader& dataLoader, const bool forceReload)
{
    const array_view<const ShaderDataLoader::ShaderVariant> shaderVariants = dataLoader.GetShaderVariants();
    if (shaderVariants.empty()) {
        return {};
    }

    const string_view compShader = shaderVariants[0].computeShader;
    if (!compShader.empty()) {
        return CreateComputeShader(dataLoader, forceReload);
    } else {
        const string_view vertShader = shaderVariants[0].vertexShader;
        const string_view fragShader = shaderVariants[0].fragmentShader;
        if (!vertShader.empty() && !fragShader.empty()) {
            return CreateGraphicsShader(dataLoader, forceReload);
        }
    }
    return {};
}

void ShaderLoader::LoadShaderStates(const string_view currentPath, const IDirectory& directory)
{
    for (auto const& entry : directory.GetEntries()) {
        switch (entry.type) {
            default:
            case IDirectory::Entry::Type::UNKNOWN:
                break;
            case IDirectory::Entry::Type::FILE: {
                HandleShaderStateFile(currentPath + entry.name, entry);
                break;
            }
            case IDirectory::Entry::Type::DIRECTORY: {
                PLUGIN_LOG_I("recursive vertex input declarations directories not supported");
                break;
            }
        }
    }
}

void ShaderLoader::CreateShaderStates(const string_view uri,
    const array_view<const ShaderStateLoaderVariantData>& variantData, const array_view<const GraphicsState>& states)
{
    for (size_t stateIdx = 0; stateIdx < states.size(); ++stateIdx) {
        const ShaderManager::GraphicsStateCreateInfo createInfo { uri, states[stateIdx] };
        const auto& variant = variantData[stateIdx];
        const ShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo { variant.renderSlot, variant.variantName,
            variant.baseShaderState, variant.baseVariantName, variant.stateFlags };
        const RenderHandleReference handle = shaderMgr_.CreateGraphicsState(createInfo, variantCreateInfo);
        if (variant.renderSlotDefaultState && (!variant.renderSlot.empty())) {
            const uint32_t renderSlotId = shaderMgr_.GetRenderSlotId(variant.renderSlot);
            shaderMgr_.SetRenderSlotData(renderSlotId, {}, handle);
        }
        if (!handle) {
            PLUGIN_LOG_E(
                "error creating graphics state (name: %s, variant: %s)", uri.data(), variant.variantName.data());
        }
    }
}

void ShaderLoader::LoadVids(const string_view currentPath, const IDirectory& directory)
{
    for (auto const& entry : directory.GetEntries()) {
        switch (entry.type) {
            default:
            case IDirectory::Entry::Type::UNKNOWN:
                break;
            case IDirectory::Entry::Type::FILE: {
                HandleVertexInputDeclarationFile(currentPath + entry.name, entry);
                break;
            }
            case IDirectory::Entry::Type::DIRECTORY: {
                PLUGIN_LOG_I("recursive vertex input declarations directories not supported");
                break;
            }
        }
    }
}

RenderHandleReference ShaderLoader::CreateVertexInputDeclaration(const VertexInputDeclarationLoader& loader)
{
    const string_view uri = loader.GetUri();
    VertexInputDeclarationView dataView = loader.GetVertexInputDeclarationView();
    return shaderMgr_.CreateVertexInputDeclaration({ uri, dataView });
}

void ShaderLoader::LoadPipelineLayouts(const string_view currentPath, const IDirectory& directory)
{
    for (auto const& entry : directory.GetEntries()) {
        switch (entry.type) {
            default:
            case IDirectory::Entry::Type::UNKNOWN:
                break;
            case IDirectory::Entry::Type::FILE: {
                HandlePipelineLayoutFile(currentPath + entry.name, entry);
                break;
            }
            case IDirectory::Entry::Type::DIRECTORY: {
                PLUGIN_LOG_I("recursive pipeline layout directories not supported");
                break;
            }
        }
    }
}

RenderHandleReference ShaderLoader::CreatePipelineLayout(const PipelineLayoutLoader& loader)
{
    const string_view uri = loader.GetUri();
    const PipelineLayout& pipelineLayout = loader.GetPipelineLayout();
    return shaderMgr_.CreatePipelineLayout({ uri, pipelineLayout });
}
RENDER_END_NAMESPACE()
