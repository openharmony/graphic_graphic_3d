#ifndef META_SRC_PROPERTY_DEPENDENCIES_H
#define META_SRC_PROPERTY_DEPENDENCIES_H

#include "property.h"

META_BEGIN_NAMESPACE()
namespace Internal {

// changing this will disable the check in property GetValue
constexpr bool ENABLE_DEPENDENCY_CHECK = true;

class Dependencies {
public:
    bool IsActive() const;

    void Start();
    void End();

    ReturnError AddDependency(const IProperty::ConstPtr& prop);
    ReturnError GetImmediateDependencies(BASE_NS::vector<IProperty::ConstPtr>& deps) const;
    bool HasDependency(const IProperty*) const;

private:
    bool active_ {};
    uint32_t depth_ {};

    ReturnError state_ { GenericError::SUCCESS };

    struct Dependancy {
        IProperty::ConstPtr property;
        uint64_t depth {};
    };
    BASE_NS::vector<Dependancy> deps_;
    BASE_NS::vector<Dependancy> deps_branch_;
};

Dependencies& GetDeps();

} // namespace Internal
META_END_NAMESPACE()

#endif