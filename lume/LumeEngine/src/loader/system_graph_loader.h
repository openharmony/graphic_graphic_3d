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

#ifndef CORE__LOADER__SYSTEM_GRAPH_LOADER_H
#define CORE__LOADER__SYSTEM_GRAPH_LOADER_H

#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
class IInterface;
class IFileManager;

class SystemGraphLoader final : public ISystemGraphLoader {
public:
    explicit SystemGraphLoader(IFileManager&);
    ~SystemGraphLoader() override = default;
    LoadResult Load(BASE_NS::string_view uri, IEcs& ecs) override;
    LoadResult LoadString(BASE_NS::string_view jsonString, IEcs& ecs) override;

protected:
    void Destroy() override;
    IFileManager& fileManager_;
};

class SystemGraphLoaderFactory final : public ISystemGraphLoaderFactory {
public:
    SystemGraphLoaderFactory() = default;
    ~SystemGraphLoaderFactory() override = default;
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;
    ISystemGraphLoader::Ptr Create(IFileManager& fileManager) override;
};
CORE_END_NAMESPACE()

#endif // CORE__LOADER__SYSTEM_GRAPH_LOADER_H
