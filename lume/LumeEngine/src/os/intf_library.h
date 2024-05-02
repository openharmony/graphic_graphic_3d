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

#ifndef API_CORE_OS_ILIBRARY_H
#define API_CORE_OS_ILIBRARY_H

#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
struct IPlugin;
/** \addtogroup group_os_ilibrary
 *  @{
 */
/** ILibrary */
class ILibrary {
public:
    ILibrary(const ILibrary& other) = delete;
    ILibrary& operator=(const ILibrary& other) = delete;

    /** Get plugin */
    virtual IPlugin* GetPlugin() const = 0;

    /** Get library file extension */
    static BASE_NS::string_view GetFileExtension();

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(ILibrary* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<ILibrary, Deleter>;

    /** Load library from given path */
    static ILibrary::Ptr Load(const BASE_NS::string_view filepath);

protected:
    ILibrary() = default;
    virtual ~ILibrary() = default;
    virtual void Destroy() = 0;
};

/** @} */
CORE_END_NAMESPACE()
#endif // API_CORE_OS_ILIBRARY_H
