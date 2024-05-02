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
#ifndef META_API_ENGINE_UTIL_H
#define META_API_ENGINE_UTIL_H

#include <meta/interface/engine/intf_engine_value_manager.h>

META_BEGIN_NAMESPACE()

inline void AddEngineValuesRecursively(
    const IEngineValueManager::Ptr& m, const IEngineValue::Ptr& value, EngineValueOptions options = {})
{
    BASE_NS::vector<IEngineValue::Ptr> vec;
    if (m->ConstructValues(
            value, EngineValueOptions { options.namePrefix, &vec, options.pushValuesDirectlyToEngine })) {
        if (options.values) {
            options.values->insert(options.values->end(), vec.begin(), vec.end());
        }
        for (auto&& v : vec) {
            AddEngineValuesRecursively(m, v, options);
        }
    }
}

inline void AddEngineValuesRecursively(
    const IEngineValueManager::Ptr& m, CORE_NS::IPropertyHandle* handle, EngineValueOptions options = {})
{
    BASE_NS::vector<IEngineValue::Ptr> vec;
    if (m->ConstructValues(
        handle, EngineValueOptions { options.namePrefix, &vec, options.pushValuesDirectlyToEngine })) {
        if (options.values) {
            options.values->insert(options.values->end(), vec.begin(), vec.end());
        }
        for (auto&& v : vec) {
            AddEngineValuesRecursively(
                m, v, EngineValueOptions { "", options.values, options.pushValuesDirectlyToEngine });
        }
    }
}

inline bool SetEngineValueToProperty(const IProperty::Ptr& p, const IEngineValue::Ptr& value)
{
    if (p && value && value->IsCompatible(p->GetTypeId())) {
        if (auto i = interface_cast<IStackProperty>(p)) {
            PropertyLock lock { p };
            TypeId ids[] = { IEngineValue::UID };
            for (auto&& v : i->GetValues(ids, false)) {
                i->RemoveValue(v);
            }
            i->PushValue(value);
            return true;
        }
    }
    return false;
}

inline IEngineValue::Ptr GetEngineValueFromProperty(const IProperty::Ptr& p)
{
    if (auto i = interface_cast<IStackProperty>(p)) {
        PropertyLock lock { p };
        TypeId ids[] = { IEngineValue::UID };
        auto values = i->GetValues(ids, false);
        if (!values.empty()) {
            return interface_pointer_cast<IEngineValue>(values.back());
        }
    }
    return nullptr;
}

META_END_NAMESPACE()

#endif
