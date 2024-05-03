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

#include "render_data_configuration_loader.h"

#include <core/io/intf_file_manager.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>

#include "json_util.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
// clang-format off
CORE_JSON_SERIALIZE_ENUM(TonemapConfiguration::TonemapType,
    {
        { static_cast<TonemapConfiguration::TonemapType>(0x7FFFFFFF), nullptr },
        { TonemapConfiguration::TonemapType::TONEMAP_ACES, "aces" },
        { TonemapConfiguration::TonemapType::TONEMAP_ACES_2020, "aces_2020" },
        { TonemapConfiguration::TonemapType::TONEMAP_FILMIC, "filmic" },
    })
CORE_JSON_SERIALIZE_ENUM(ColorConversionConfiguration::ConversionFunctionType,
    {
        { static_cast<ColorConversionConfiguration::ConversionFunctionType>(0x7FFFFFFF), nullptr },
        { ColorConversionConfiguration::ConversionFunctionType::CONVERSION_LINEAR, "linear" },
        { ColorConversionConfiguration::ConversionFunctionType::CONVERSION_LINEAR_TO_SRGB, "linear_to_srgb" },
    })
CORE_JSON_SERIALIZE_ENUM(DitherConfiguration::DitherType,
    {
        { static_cast<DitherConfiguration::DitherType>(0x7FFFFFFF), nullptr },
        { DitherConfiguration::DitherType::INTERLEAVED_NOISE, "interleaved_noise" },
        { DitherConfiguration::DitherType::TRIANGLE_NOISE, "triangle_noise" },
        { DitherConfiguration::DitherType::TRIANGLE_NOISE_RGB, "triangle_noise_rgb" },
    })
CORE_JSON_SERIALIZE_ENUM(BlurConfiguration::BlurQualityType,
    {
        { static_cast<BlurConfiguration::BlurQualityType>(0x7FFFFFFF), nullptr },
        { BlurConfiguration::BlurQualityType::QUALITY_TYPE_LOW, "low" },
        { BlurConfiguration::BlurQualityType::QUALITY_TYPE_NORMAL, "normal" },
        { BlurConfiguration::BlurQualityType::QUALITY_TYPE_HIGH, "high" },
    })
CORE_JSON_SERIALIZE_ENUM(BlurConfiguration::BlurType,
    {
        { static_cast<BlurConfiguration::BlurType>(0x7FFFFFFF), nullptr },
        { BlurConfiguration::BlurType::TYPE_NORMAL, "normal" },
        { BlurConfiguration::BlurType::TYPE_HORIZONTAL, "horizontal" },
        { BlurConfiguration::BlurType::TYPE_VERTICAL, "vertical" },
    })
CORE_JSON_SERIALIZE_ENUM(BloomConfiguration::BloomQualityType,
    {
        { static_cast<BloomConfiguration::BloomQualityType>(0x7FFFFFFF), nullptr },
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_LOW, "low" },
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_NORMAL, "normal" },
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_HIGH, "high" },
    })
CORE_JSON_SERIALIZE_ENUM(BloomConfiguration::BloomType,
    {
        { static_cast<BloomConfiguration::BloomType>(0x7FFFFFFF), nullptr },
        { BloomConfiguration::BloomType::TYPE_NORMAL, "normal" },
        { BloomConfiguration::BloomType::TYPE_HORIZONTAL, "horizontal" },
        { BloomConfiguration::BloomType::TYPE_VERTICAL, "vertical" },
        { BloomConfiguration::BloomType::TYPE_BILATERAL, "bilateral" },
    })
CORE_JSON_SERIALIZE_ENUM(FxaaConfiguration::Sharpness,
    {
        { static_cast<FxaaConfiguration::Sharpness>(0x7FFFFFFF), nullptr },
        { FxaaConfiguration::Sharpness::SOFT, "soft" },
        { FxaaConfiguration::Sharpness::MEDIUM, "medium" },
        { FxaaConfiguration::Sharpness::SHARP, "sharp" },
    })
CORE_JSON_SERIALIZE_ENUM(FxaaConfiguration::Quality,
    {
        { static_cast<FxaaConfiguration::Quality>(0x7FFFFFFF), nullptr },
        { FxaaConfiguration::Quality::LOW, "low" },
        { FxaaConfiguration::Quality::MEDIUM, "medium" },
        { FxaaConfiguration::Quality::HIGH, "high" },
    })
CORE_JSON_SERIALIZE_ENUM(PostProcessConfiguration::PostProcessEnableFlagBits,
    {
        { static_cast<PostProcessConfiguration::PostProcessEnableFlagBits>(0x7FFFFFFF), nullptr },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TONEMAP_BIT, "tonemap" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_VIGNETTE_BIT, "vignette" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_DITHER_BIT, "dither" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_COLOR_CONVERSION_BIT, "color_conversion" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_COLOR_FRINGE_BIT, "color_fringe" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT, "blur" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT, "bloom" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_FXAA_BIT, "fxaa" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TAA_BIT, "taa" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_DOF_BIT, "dof" },
        { PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_MOTION_BLUR_BIT, "motion_blur" },
    })
// clang-format on
namespace {
IRenderDataConfigurationLoader::LoadedPostProcess LoadPostProcess(const json::value& jsonData)
{
    IRenderDataConfigurationLoader::LoadedPostProcess result;
    auto& ppConfig = result.postProcessConfiguration;
    auto& err = result.loadResult.error;

    SafeGetJsonValue(jsonData, "name", err, result.name);
    float version = 0.0f;
    SafeGetJsonValue(jsonData, "version", err, version);
    if (version < 1.0f) {
        PLUGIN_LOG_W("version number should be 1.0 or higher for post process configuration json (version=%f name=%s)",
            version, result.name.c_str());
    }
    if (err.empty()) {
        if (const auto iter = jsonData.find("postProcessConfiguration"); iter) {
            SafeGetJsonBitfield<PostProcessConfiguration::PostProcessEnableFlagBits>(
                *iter, "enableFlags", err, ppConfig.enableFlags);

            if (const auto cIter = iter->find("bloomConfiguration"); cIter) {
                SafeGetJsonEnum(*cIter, "bloomType", err, ppConfig.bloomConfiguration.bloomType);
                SafeGetJsonEnum(*cIter, "bloomQualityType", err, ppConfig.bloomConfiguration.bloomQualityType);
                SafeGetJsonValue(*cIter, "thresholdHard", err, ppConfig.bloomConfiguration.thresholdHard);
                SafeGetJsonValue(*cIter, "thresholdSoft", err, ppConfig.bloomConfiguration.thresholdSoft);
                SafeGetJsonValue(*cIter, "amountCoefficient", err, ppConfig.bloomConfiguration.amountCoefficient);
                SafeGetJsonValue(*cIter, "dirtMaskCoefficient", err, ppConfig.bloomConfiguration.dirtMaskCoefficient);
                SafeGetJsonValue(*cIter, "useCompute", err, ppConfig.bloomConfiguration.useCompute);
                // NOTE: dirt mask name should be added
            }
            if (const auto cIter = iter->find("vignetteConfiguration"); cIter) {
                SafeGetJsonValue(*cIter, "coefficient", err, ppConfig.vignetteConfiguration.coefficient);
                SafeGetJsonValue(*cIter, "power", err, ppConfig.vignetteConfiguration.power);
            }
            if (const auto cIter = iter->find("colorFringeConfiguration"); cIter) {
                SafeGetJsonValue(
                    *cIter, "distanceCoefficient", err, ppConfig.colorFringeConfiguration.distanceCoefficient);
                SafeGetJsonValue(*cIter, "coefficient", err, ppConfig.colorFringeConfiguration.coefficient);
            }
            if (const auto cIter = iter->find("tonemapConfiguration"); cIter) {
                SafeGetJsonEnum(*cIter, "tonemapType", err, ppConfig.tonemapConfiguration.tonemapType);
                SafeGetJsonValue(*cIter, "exposure", err, ppConfig.tonemapConfiguration.exposure);
            }
            if (const auto cIter = iter->find("ditherConfiguration"); cIter) {
                SafeGetJsonEnum(*cIter, "ditherType", err, ppConfig.ditherConfiguration.ditherType);
            }
            if (const auto cIter = iter->find("blurConfiguration"); cIter) {
                SafeGetJsonEnum(*cIter, "blurType", err, ppConfig.blurConfiguration.blurType);
                SafeGetJsonEnum(*cIter, "blurQualityType", err, ppConfig.blurConfiguration.blurQualityType);
                SafeGetJsonValue(*cIter, "filterSize", err, ppConfig.blurConfiguration.filterSize);
                SafeGetJsonValue(*cIter, "maxMipLevel", err, ppConfig.blurConfiguration.maxMipLevel);
            }
            if (const auto cIter = iter->find("colorConversionConfiguration"); cIter) {
                SafeGetJsonEnum(*cIter, "conversionFunctionType", err,
                    ppConfig.colorConversionConfiguration.conversionFunctionType);
            }
            if (const auto cIter = iter->find("FxaaConfiguration"); cIter) {
                SafeGetJsonEnum(*cIter, "sharpness", err, ppConfig.fxaaConfiguration.sharpness);
                SafeGetJsonEnum(*cIter, "quality", err, ppConfig.fxaaConfiguration.quality);
            }
        } else {
            err += "postProcessConfiguration not found\n";
        }
    }
    result.loadResult.success = err.empty();

    return result;
}
} // namespace

IRenderDataConfigurationLoader::LoadedPostProcess RenderDataConfigurationLoader::LoadPostProcess(
    const string_view jsonString)
{
    IRenderDataConfigurationLoader::LoadedPostProcess result;
    auto json = json::parse(jsonString.data());
    if (json) {
        result = RENDER_NS::LoadPostProcess(json);
    } else {
        result.loadResult.success = false;
        result.loadResult.error = "Invalid json file.";
    }

    return result;
}

IRenderDataConfigurationLoader::LoadedPostProcess RenderDataConfigurationLoader::LoadPostProcess(
    IFileManager& fileManager, const string_view uri)
{
    IRenderDataConfigurationLoader::LoadedPostProcess result;

    IFile::Ptr file = fileManager.OpenFile(uri);
    if (!file) {
        PLUGIN_LOG_E("Error loading '%s'", string(uri).c_str());
        result.loadResult = IRenderDataConfigurationLoader::LoadResult("Failed to open file.");
        return result;
    }

    const uint64_t byteLength = file->GetLength();

    string raw;
    raw.resize(static_cast<size_t>(byteLength));

    if (file->Read(raw.data(), byteLength) != byteLength) {
        PLUGIN_LOG_E("Error loading '%s'", string(uri).c_str());
        result.loadResult = IRenderDataConfigurationLoader::LoadResult("Failed to read file.");
        return result;
    }

    return LoadPostProcess(raw);
}

IRenderDataConfigurationLoader::LoadedPostProcess RenderDataConfigurationLoaderImpl::LoadPostProcess(
    const string_view jsonString)
{
    return RenderDataConfigurationLoader::LoadPostProcess(jsonString);
}

IRenderDataConfigurationLoader::LoadedPostProcess RenderDataConfigurationLoaderImpl::LoadPostProcess(
    IFileManager& fileManager, const string_view uri)
{
    return RenderDataConfigurationLoader::LoadPostProcess(fileManager, uri);
}

const IInterface* RenderDataConfigurationLoaderImpl::GetInterface(const BASE_NS::Uid& uid) const
{
    if ((uid == IRenderDataConfigurationLoader::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* RenderDataConfigurationLoaderImpl::GetInterface(const BASE_NS::Uid& uid)
{
    if ((uid == IRenderDataConfigurationLoader::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void RenderDataConfigurationLoaderImpl::Ref() {}

void RenderDataConfigurationLoaderImpl::Unref() {}
RENDER_END_NAMESPACE()
