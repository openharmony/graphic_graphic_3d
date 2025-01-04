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
