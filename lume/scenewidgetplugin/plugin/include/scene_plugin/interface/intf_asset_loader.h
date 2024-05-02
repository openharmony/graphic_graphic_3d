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

#ifndef SCENEPLUGIN_INTF_ASSETLOADER_H
#define SCENEPLUGIN_INTF_ASSETLOADER_H

#include <scene_plugin/interface/intf_entity_collection.h>

#include <base/containers/string.h>

SCENE_BEGIN_NAMESPACE()

/** Asset loader interface */
class IAssetLoader {
public:
    /** Listener for asset loading events. */
    class IListener {
    public:
        /** On load finished */
        virtual void OnLoadFinished(const IAssetLoader& loader) = 0;

    protected:
        virtual ~IListener() = default;
    };

    /** Describes result of the loading operation. */
    struct Result {
        /** Indicates, whether the loading operation is successful. */
        bool success { true };

        /** In case of a loading error, contains the description of the error. */
        BASE_NS::string error;
    };

    /** Returns the entity collection used by this loader. */
    virtual IEntityCollection& GetEntityCollection() const = 0;

    /** Returns the src for this loader that is used to resolve the final loading uri. */
    virtual BASE_NS::string GetSrc() const = 0;

    /** Returns the context uri for this loader. The src value is resolved using this context to get the actual uri. */
    virtual BASE_NS::string GetContextUri() const = 0;

    /** Returns the resolved uri that is used for loading. */
    virtual BASE_NS::string GetUri() const = 0;

    /** Add a listener that gets callbacks about loading events. */
    virtual void AddListener(IListener& listener) = 0;

    /** Add a listener that gets callbacks about loading events. */
    virtual void RemoveListener(IListener& listener) = 0;

    /** Load asset data synchronously. The previous loaded data will be discarded. */
    virtual void LoadAsset() = 0;

    /** Load asset data asynchronously, user is required to call Execute() from main thread until it returns true.
     * The previous loaded data will be discarded. */
    virtual void LoadAssetAsync() = 0;

    /** Advances the loading process, needs to be called from the main thread when performing asynchronous loading.
     *  @param timeBudget Time budget for resource loading in microseconds, if 0 all available work will be executed
     *  during this frame.
     *  @return true if the task has completed.
     */
    virtual bool Execute(uint32_t timeBudget) = 0;

    /** Cancel the loading operation */
    virtual void Cancel() = 0;

    /** Returns true if the loading process has been completed. */
    virtual bool IsCompleted() const = 0;

    /** Returns the status of the loading operation. */
    virtual Result GetResult() const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IAssetLoader* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IAssetLoader, Deleter>;

protected:
    IAssetLoader() = default;
    virtual ~IAssetLoader() = default;
    virtual void Destroy() = 0;
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_INTF_IASSETLOADER_H
