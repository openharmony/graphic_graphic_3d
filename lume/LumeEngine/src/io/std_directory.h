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

#ifndef CORE__IO__STD_DIRECTORY_H
#define CORE__IO__STD_DIRECTORY_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
struct DirImpl;

class StdDirectory final : public IDirectory {
public:
    StdDirectory(BASE_NS::unique_ptr<DirImpl>);
    ~StdDirectory() override;

    static IDirectory::Ptr Create(BASE_NS::string_view path);
    static IDirectory::Ptr Open(BASE_NS::string_view path);
    void Close() override;

    BASE_NS::vector<Entry> GetEntries() const override;

    // NOTE: Helper function for resolving paths does not really belong here but it's here
    // so that msdirent.h is not needed in multiple compilation units.
    static BASE_NS::string ResolveAbsolutePath(BASE_NS::string_view path, bool isDirectory);
    static void FormatPath(BASE_NS::string& path, bool isDirectory);
    static BASE_NS::string GetDirName(BASE_NS::string_view path);
    static BASE_NS::string GetBaseName(BASE_NS::string_view path);

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    BASE_NS::unique_ptr<DirImpl> dir_;
};
CORE_END_NAMESPACE()

#endif // CORE__IO__STD_DIRECTORY_H
