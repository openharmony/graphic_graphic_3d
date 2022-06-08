/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
