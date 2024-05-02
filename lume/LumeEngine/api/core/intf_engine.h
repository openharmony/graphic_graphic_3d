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

#ifndef API_CORE_IENGINE_H
#define API_CORE_IENGINE_H

#include <cstdint>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/ecs/intf_ecs.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_factory.h>
#include <core/plugin/intf_interface.h>

BASE_BEGIN_NAMESPACE()
template<class T>
class array_view;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
struct EngineCreateInfo;
class IFileManager;
class IImageLoaderManager;
class IPlatform;
class IThreadPool;

/** \addtogroup group_iengine
 *  @{
 */
/** Engine time (totaltime and deltatime in microseconds) */
struct EngineTime {
    /** Total time */
    uint64_t totalTimeUs { 0 };
    /** Delta time */
    uint64_t deltaTimeUs { 0 };
};

/** Engine interface.
    Engine needs to be created with IEngineFactory::Create.
*/
class IEngine : public IClassFactory {
public:
    static constexpr BASE_NS::Uid UID { "760877f7-0baf-422b-a1a7-35834683ddd3" };

    using Ptr = BASE_NS::refcnt_ptr<IEngine>;

    /** Init engine. Create needed managers and data.
     *  Needs to be called before accessing managers.
     */
    virtual void Init() = 0;

    /** Tick frame. Update ECS scene.
     *  Needs to be called once per frame before RenderFrame().
     *  @return True if ECS updated render data stores and rendering is required.
     */
    virtual bool TickFrame() = 0;

    /** Tick frame. Update internal and given ECS scenes.
     *  Needs to be called once per frame before RenderFrame().
     *  @param ecsInputs ECS instances that need to be updated.
     *  @return True if rendering is required.
     */
    virtual bool TickFrame(const BASE_NS::array_view<IEcs*>& ecsInputs) = 0;

    /** Get file manager */
    virtual IFileManager& GetFileManager() = 0;

    /** Return platform specific information */
    virtual const IPlatform& GetPlatform() const = 0;

    /** Get engine time.
     *  @return EngineTime struct.
     */
    virtual EngineTime GetEngineTime() const = 0;

    /** Creates a new ECS instance.
     * @return ECS instance.
     */
    virtual IEcs::Ptr CreateEcs() = 0;

    /** Creates a new ECS instance with a shared thread pool.
     * @param threadPool Thread pool which the ECS instance and it's managers and systems can use.
     * @return ECS instance.
     */
    virtual IEcs::Ptr CreateEcs(IThreadPool& threadPool) = 0;

    virtual IImageLoaderManager& GetImageLoaderManager() = 0;

    /** Get version */
    virtual BASE_NS::string_view GetVersion() = 0;

    /** Get whether engine is build in debug mode */
    virtual bool IsDebugBuild() = 0;

protected:
    IEngine() = default;
    virtual ~IEngine() = default;
};

inline constexpr BASE_NS::string_view GetName(const IEngine*)
{
    return "IEngine";
}

// factory inteface.
class IEngineFactory : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "f2ce87a4-5a3d-4327-bc90-ff29efb398b0" };

    using Ptr = BASE_NS::refcnt_ptr<IEngineFactory>;

    virtual IEngine::Ptr Create(const EngineCreateInfo& engineCreateInfo) = 0;

protected:
    IEngineFactory() = default;
    virtual ~IEngineFactory() = default;
};

inline constexpr BASE_NS::string_view GetName(const IEngineFactory*)
{
    return "IEngineFactory";
}

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)
/** Get version */
extern BASE_NS::string_view (*GetVersion)();
/** Get whether engine is build in debug mode */
extern bool (*IsDebugBuild)();
#else
/** Get version */
CORE_PUBLIC BASE_NS::string_view GetVersion();
/** Get whether engine is build in debug mode */
CORE_PUBLIC bool IsDebugBuild();
#endif
/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_IENGINE_H
