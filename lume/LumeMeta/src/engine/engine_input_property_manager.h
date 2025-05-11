#ifndef META_SRC_ENGINE_ENGINE_INPUT_PROPERTY_MANAGER_H
#define META_SRC_ENGINE_ENGINE_INPUT_PROPERTY_MANAGER_H

#include <meta/interface/engine/intf_engine_input_property_manager.h>

#include "engine_value_manager.h"

META_BEGIN_NAMESPACE()

namespace Internal {

class EngineInputPropertyManager : public IntroduceInterfaces<BaseObject, IEngineInputPropertyManager> {
    META_OBJECT(EngineInputPropertyManager, META_NS::ClassId::EngineInputPropertyManager, IntroduceInterfaces)
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