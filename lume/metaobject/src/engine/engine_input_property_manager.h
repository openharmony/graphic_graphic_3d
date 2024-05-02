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
#ifndef META_SRC_ENGINE_ENGINE_INPUT_PROPERTY_MANAGER_H
#define META_SRC_ENGINE_ENGINE_INPUT_PROPERTY_MANAGER_H

#include <meta/interface/engine/intf_engine_input_property_manager.h>

#include "engine_value_manager.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class EngineInputPropertyManager : public Internal::BaseObjectFwd<EngineInputPropertyManager,
                                       META_NS::ClassId::EngineInputPropertyManager, IEngineInputPropertyManager> {
public:
    bool Build(const IMetadata::Ptr& data) override;

    bool Sync() override;

    IProperty::Ptr ConstructProperty(BASE_NS::string_view name) override;
    IProperty::Ptr TieProperty(const IProperty::Ptr&, BASE_NS::string valueName) override;
    BASE_NS::vector<IProperty::Ptr> GetAllProperties() const override;
    bool PopulateProperties(IMetadata&) override;
    void RemoveProperty(BASE_NS::string_view name) override;

    IEngineValueManager::Ptr GetValueManager() const override;

private:
    mutable std::shared_mutex mutex_;
    struct PropInfo {
        IProperty::Ptr property;
        IEngineValue::Ptr value;
    };
    IEngineValueManager::Ptr manager_;
    BASE_NS::unordered_map<BASE_NS::string, PropInfo> props_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif