/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_IFILE_MONITOR_H
#define API_CORE_IFILE_MONITOR_H

#include <cstdint>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

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
