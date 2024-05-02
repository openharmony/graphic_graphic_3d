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

#ifndef API_CORE_IFILE_MONITOR_H
#define API_CORE_IFILE_MONITOR_H

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

BASE_BEGIN_NAMESPACE()
template<typename T>
class vector;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IFileManager;

/** @ingroup group_ifilemonitor */
/** Filemonitor interface
 *  Helper class to monitor changes in filesystems, in hot-reload scenarios.
 *  Usage:
 */

class IFileMonitor : public IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "107c2b9f-3be3-4d56-be22-6e3259a486f5" };

    using Ptr = BASE_NS::refcnt_ptr<IFileMonitor>;

    virtual void Initialize(IFileManager&) = 0;
    virtual bool AddPath(BASE_NS::string_view path) = 0;
    virtual bool RemovePath(BASE_NS::string_view path) = 0;
    virtual void ScanModifications(BASE_NS::vector<BASE_NS::string>& added, BASE_NS::vector<BASE_NS::string>& removed,
        BASE_NS::vector<BASE_NS::string>& modified) = 0;

protected:
    IFileMonitor() = default;
    virtual ~IFileMonitor() = default;
};

inline constexpr BASE_NS::string_view GetName(const IFileMonitor*)
{
    return "IFileMonitor";
}
CORE_END_NAMESPACE()

#endif // API_CORE_IFILE_MONITOR_H
