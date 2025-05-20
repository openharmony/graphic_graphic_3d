/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_IRENDER_CONTEXT_H
#define API_RENDER_IRENDER_CONTEXT_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/util/color.h>
#include <core/plugin/intf_class_factory.h>
#include <render/device/intf_device.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

CORE_BEGIN_NAMESPACE()
class IEngine;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class IRenderDataStoreManager;
class IRenderNodeGraphManager;
class IRenderer;
class IRenderUtil;

/**
 * RenderResultCode
 */
enum class RenderResultCode : uint32_t {
    RENDER_SUCCESS = 0,
    RENDER_ERROR = 1,
};

/**
 * VersionInfo
 */
struct VersionInfo {
    /** Name, default is "no_name" */
    BASE_NS::string name { "no_name" };
    /** Major number of version */
    uint32_t versionMajor { 0 };
    /** Minor number of version */
    uint32_t versionMinor { 0 };
    /** Patch number of version */
    uint32_t versionPatch { 0 };
};

/**
 * RenderCreateInfo
 */
struct RenderCreateInfo {
    /** Create info flag bits to setup configuration flags */
    enum CreateInfoFlagBits : uint32_t {
        /** Request separate render frame backend (backend graphics API control) */
        CREATE_INFO_SEPARATE_RENDER_FRAME_BACKEND_BIT = 0x00000002,
        /** Request separate render frame present (backend graphics API presentation) */
        CREATE_INFO_SEPARATE_RENDER_FRAME_PRESENT_BIT = 0x00000004,
        /** Request ray-tracing support */
        CREATE_INFO_RAY_TRACING_BIT = 0x00000008,
    };
    /** Container for render create info flag bits */
    using CreateInfoFlags = uint32_t;
    /** Thread pool create info */
    struct ThreadPoolCreateInfo {
        /** Coefficient to hardware registered number of cores
         * With 1.0f and maxCount { ~0U }, the maximum number of core tasks are in use
         */
        float threadCountCoefficient { 0.5f };
        /** Minimum count of threads in the thread pool.
         * The renderer should have at least 4 threads to parallelize correctly.
         */
        uint32_t minCount { 4U };
        /** Maximum count of threads in the thread pool.
         * The renderer thread count should not exceed the expected task count.
         */
        uint32_t maxCount { ~0U };
    };

    /** Application version info */
    VersionInfo applicationInfo;
    /** Device create info */
    DeviceCreateInfo deviceCreateInfo;
    /** Creation flags */
    CreateInfoFlags createFlags { 0U };
    /** Threadpool create info */
    ThreadPoolCreateInfo threadPoolCreateInfo;
    /** Color space flags (defaults to linear) */
    BASE_NS::ColorSpaceFlags colorSpaceFlags { 0U };
};

/**
 * IRenderContext interface for accessing managers and renderer.
 * Device must be created to access valid renderer, render node graph manager, and render data store manager.
 */
class IRenderContext : public CORE_NS::IClassFactory {
public:
    static constexpr auto UID = BASE_NS::Uid { "c8d8650b-efac-4e04-8e66-e12b35d2749e" };
    using Ptr = BASE_NS::refcnt_ptr<IRenderContext>;

    /** Init, create device for render use.
     *  @param createInfo Device creation info.
     */
    virtual RenderResultCode Init(const RenderCreateInfo& createInfo) = 0;

    /** Get active device. */
    virtual IDevice& GetDevice() const = 0;
    /** Get rendererer */
    virtual IRenderer& GetRenderer() const = 0;
    /** Get render node graph manager */
    virtual IRenderNodeGraphManager& GetRenderNodeGraphManager() const = 0;
    /** Get render data store manager */
    virtual IRenderDataStoreManager& GetRenderDataStoreManager() const = 0;

    /** Get render utilities */
    virtual IRenderUtil& GetRenderUtil() const = 0;

    /** Get engine */
    virtual CORE_NS::IEngine& GetEngine() const = 0;

    /** Get version */
    virtual BASE_NS::string_view GetVersion() = 0;

    /** Writes current pipelines to disk if cache:// location exists. */
    virtual void WritePipelineCache() const = 0;

    /** Get create info */
    virtual RenderCreateInfo GetCreateInfo() const = 0;

protected:
    IRenderContext() = default;
    virtual ~IRenderContext() = default;
};

inline constexpr BASE_NS::string_view GetName(const IRenderContext*)
{
    return "IRenderContext";
}
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_CONTEXT_H
