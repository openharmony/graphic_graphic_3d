/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_PLUGIN_ICLASS_INFO_H
#define API_CORE_PLUGIN_ICLASS_INFO_H

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
/** Interface for checking implementation UID and name.
 */
class IClassInfo : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "237773cf-7a76-4e39-a067-f454183b036c" };

    using Ptr = BASE_NS::refcnt_ptr<IClassInfo>;

    /** Returns the UID of the class which implements this interface.
     */
    virtual BASE_NS::Uid GetClassUid() const = 0;

    /** Returns the name of the class which implements this interface.
     */
    virtual BASE_NS::string_view GetClassName() const = 0;

protected:
    IClassInfo() = default;
    virtual ~IClassInfo() = default;
};
CORE_END_NAMESPACE()

#endif // API_CORE_PLUGIN_ICLASS_INFO_H
