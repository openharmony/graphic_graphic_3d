/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_ENGINE_FACTORY_H
#define CORE_ENGINE_FACTORY_H

#include <cstdint>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string_view.h>
#include <core/intf_engine.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_info.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
class EngineFactory final : public IEngineFactory, public IClassInfo {
public:
    EngineFactory() = default;
    virtual ~EngineFactory() override = default;

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

    // IClassInfo
    BASE_NS::Uid GetClassUid() const override;
    BASE_NS::string_view GetClassName() const override;

    // IEngineFactory
    IEngine::Ptr Create(const EngineCreateInfo& engineCreateInfo) override;
    uint32_t refcnt_ { 0 };
};
CORE_END_NAMESPACE()

#endif // CORE_ENGINE_FACTORY_H
