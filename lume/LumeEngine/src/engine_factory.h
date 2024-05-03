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

#ifndef CORE_ENGINE_FACTORY_H
#define CORE_ENGINE_FACTORY_H

#include <cstdint>

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <core/intf_engine.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_info.h>

CORE_BEGIN_NAMESPACE()
class IInterface;
struct EngineCreateInfo;
class EngineFactory final : public IEngineFactory, public IClassInfo {
public:
    EngineFactory() = default;
    ~EngineFactory() override = default;

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
