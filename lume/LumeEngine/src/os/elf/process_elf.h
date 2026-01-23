/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OS_PROCESS_ELF_H
#define OS_PROCESS_ELF_H

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/io/intf_file.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
struct PluginData {
    BASE_NS::string filename;
    BASE_NS::Uid pluginUid;
    BASE_NS::vector<BASE_NS::Uid> dependencies;
};

PluginData ProcessElf(BASE_NS::string_view filePath, const IFile::Ptr& filePtr);
CORE_END_NAMESPACE()
#endif // OS_PROCESS_ELF_H