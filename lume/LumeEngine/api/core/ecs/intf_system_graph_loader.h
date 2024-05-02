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

#ifndef API_CORE_ECS_ISYSTEM_GRAPH_H
#define API_CORE_ECS_ISYSTEM_GRAPH_H

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
class IEcs;

/** @ingroup group_ecs_isystemgraph */
/**
 * ISystemGraphLoader.
 * Interface class for creating a SystemGraphDesc from a JSON file.
 */
class ISystemGraphLoader {
public:
    /** Describes result of the loading operation. */
    struct LoadResult {
        LoadResult() = default;
        explicit LoadResult(BASE_NS::string&& error) : success(false), error(BASE_NS::move(error)) {}

        /** Indicates, whether the parsing operation is successful. */
        bool success { true };

        /** In case of parsing error, contains the description of the error. */
        BASE_NS::string error;
    };

    /** Load a system graph description from a file.
     *  @param uri URI to a file containing the system graph description in JSON format.
     *  @param ecs Ecs instance to populate with systems and components.
     *  @return If loading fails LoadResult::success is false and LoadResult::error contains an error message.
     */
    virtual LoadResult Load(BASE_NS::string_view uri, IEcs& ecs) = 0;

    /** Load a system graph description from a JSON string.
     *  @param json String containing the system graph description in JSON format.
     *  @param ecs Ecs instance to populate with systems and components.
     *  @return If loading fails LoadResult::success is false and LoadResult::error contains an error message.
     */
    virtual LoadResult LoadString(BASE_NS::string_view json, IEcs& ecs) = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(ISystemGraphLoader* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<ISystemGraphLoader, struct Deleter>;

protected:
    ISystemGraphLoader() = default;
    virtual ~ISystemGraphLoader() = default;
    virtual void Destroy() = 0;
};

// factory inteface.
class ISystemGraphLoaderFactory : public IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "26f8f9de-53e9-4a09-85a6-45b69eb3efb6" };

    using Ptr = BASE_NS::refcnt_ptr<ISystemGraphLoaderFactory>;

    virtual ISystemGraphLoader::Ptr Create(IFileManager& fileManager) = 0;

protected:
    ISystemGraphLoaderFactory() = default;
    virtual ~ISystemGraphLoaderFactory() = default;
};

inline constexpr BASE_NS::string_view GetName(const ISystemGraphLoaderFactory*)
{
    return "ISystemGraphLoaderFactory";
}
CORE_END_NAMESPACE()

#endif // API_CORE_ECS_ISYSTEM_GRAPH_H
