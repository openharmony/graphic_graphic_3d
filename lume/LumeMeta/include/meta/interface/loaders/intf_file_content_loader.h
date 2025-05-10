/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: File content loader interface
 * Author: Lauri Jaaskela
 * Create: 2022-05-30
 */

#ifndef META_INTERFACE_LOADERS_FILE_CONTENT_LOADER_H
#define META_INTERFACE_LOADERS_FILE_CONTENT_LOADER_H

#include <base/containers/string.h>
#include <core/io/intf_file.h>

#include <meta/interface/loaders/intf_dynamic_content_loader.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IFileContentLoader, "4bcb1a14-9022-4f5c-91fd-67ad36a87a4e")

/**
 * @brief The IFileContentLoader interface defines an interface for content loaders
 *        which load their content from a file.
 */
class IFileContentLoader : public IDynamicContentLoader {
public:
    META_INTERFACE(IDynamicContentLoader, IFileContentLoader)
public:
    /**
     * @brief File to load.
     */
    virtual bool SetFile(CORE_NS::IFile::Ptr) = 0;
    /**
     * @brief If true, content loader should implement caching of the source file.
     *        The default value is false.
     */
    META_PROPERTY(bool, Cached)
};

META_END_NAMESPACE()

#endif
