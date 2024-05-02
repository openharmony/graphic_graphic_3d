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
#include "engine_input_property_manager.h"

META_BEGIN_NAMESPACE()

namespace Internal {

bool EngineInputPropertyManager::Build(const IMetadata::Ptr& data)
{
    manager_ = GetObjectRegistry().Create<IEngineValueManager>(META_NS::ClassId::EngineValueManager);
    return manager_ != nullptr;
}
bool EngineInputPropertyManager::Sync()
{
    std::unique_lock lock { mutex_ };
    for (auto&& p : props_) {
        // evaluate property and set the value to engine value for synchronisation
        p.second.value->SetValue(p.second.property->GetValue());
    }
    return manager_->Sync(EngineSyncDirection::TO_ENGINE);
}
static IProperty::Ptr ConstructFromValue(const IEngineValue::Ptr& value)
{
    IProperty::Ptr property;
    if (auto internal = interface_cast<IEngineValueInternal>(value)) {
        property = GetObjectRegistry().GetPropertyRegister().Create(META_NS::ClassId::StackProperty, value->GetName());
        if (auto i = interface_cast<IPropertyInternalAny>(property)) {
            i->SetInternalAny(internal->GetInternalAccess()->CreateAny());
        }
        if (auto i = interface_cast<IStackProperty>(property)) {
            i->SetDefaultValue(value->GetValue());
        }
    }
    return property;
}
IProperty::Ptr EngineInputPropertyManager::ConstructProperty(BASE_NS::string_view name)
{
    std::unique_lock lock { mutex_ };
    auto it = props_.find(name);
    if (it != props_.end()) {
        return it->second.property;
    }
    auto value = manager_->GetEngineValue(name);
    IProperty::Ptr property = ConstructFromValue(value);
    if (property) {
        props_[name] = PropInfo { property, value };
    }
    return property;
}
IProperty::Ptr EngineInputPropertyManager::TieProperty(const IProperty::Ptr& p, BASE_NS::string valueName)
{
    if (valueName.empty()) {
        valueName = p->GetName();
    }
    std::unique_lock lock { mutex_ };
    auto it = props_.find(p->GetName());
    IEngineValue::Ptr value;
    if (it != props_.end()) {
        value = it->second.value;
    }
    if (!value) {
        value = manager_->GetEngineValue(valueName);
    }
    if (value && value->IsCompatible(p->GetTypeId())) {
        props_[p->GetName()] = PropInfo { p, value };
        return p;
    }
    return nullptr;
}
BASE_NS::vector<IProperty::Ptr> EngineInputPropertyManager::GetAllProperties() const
{
    BASE_NS::vector<IProperty::Ptr> ret;
    std::shared_lock lock { mutex_ };
    for (auto&& p : props_) {
        ret.push_back(p.second.property);
    }
    return ret;
}
bool EngineInputPropertyManager::PopulateProperties(IMetadata& data)
{
    std::unique_lock lock { mutex_ };
    props_.clear();
    for (auto&& v : manager_->GetAllEngineValues()) {
        if (auto p = data.GetPropertyByName(v->GetName())) {
            if (v->IsCompatible(p->GetTypeId())) {
                props_[v->GetName()] = PropInfo { p, v };
            }
        }
        if (auto p = ConstructFromValue(v)) {
            props_[v->GetName()] = PropInfo { p, v };
            data.AddProperty(p);
        }
    }
    return true;
}
void EngineInputPropertyManager::RemoveProperty(BASE_NS::string_view name)
{
    std::unique_lock lock { mutex_ };
    props_.erase(name);
}
IEngineValueManager::Ptr EngineInputPropertyManager::GetValueManager() const
{
    std::shared_lock lock { mutex_ };
    return manager_;
}

} // namespace Internal

META_END_NAMESPACE()
