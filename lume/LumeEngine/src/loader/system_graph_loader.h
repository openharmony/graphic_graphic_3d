/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__LOADER__SYSTEM_GRAPH_LOADER_H
#define CORE__LOADER__SYSTEM_GRAPH_LOADER_H

#include <base/containers/array_view.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEngine;
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
