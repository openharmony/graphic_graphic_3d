#include "dependencies.h"

META_BEGIN_NAMESPACE()
namespace Internal {

bool Dependencies::IsActive() const
{
    return active_;
}
void Dependencies::Start()
{
    if (active_) {
        ++depth_;
    } else {
        active_ = true;
        depth_ = 1;
    }
}
void Dependencies::End()
{
    if (active_) {
        if (--depth_ == 0) {
            active_ = false;
            deps_.clear();
            state_ = GenericError::SUCCESS;
        }
    }
}
ReturnError Dependencies::AddDependency(const IProperty::ConstPtr& prop)
{
    if (active_ && prop) {
        // save dependencies only once per property/depth
        for (const auto& b : deps_) {
            if (b.depth == depth_ && b.property == prop) {
                return GenericError::SUCCESS;
            }
        }
        deps_.push_back({ prop, depth_ });
    }
    return GenericError::SUCCESS;
}
ReturnError Dependencies::GetImmediateDependencies(BASE_NS::vector<IProperty::ConstPtr>& deps) const
{
    if (state_) {
        BASE_NS::vector<IProperty::ConstPtr> immediate;
        for (auto&& v : deps_) {
            if (v.depth == depth_) {
                immediate.push_back(v.property);
            }
        }
        deps = BASE_NS::move(immediate);
    }
    return state_;
}

bool Dependencies::HasDependency(const IProperty* p) const
{
    for (auto&& v : deps_) {
        if (v.depth >= depth_ && v.property.get() == p) {
            return true;
        }
    }
    return false;
}

Dependencies& GetDeps()
{
    thread_local Dependencies deps;
    return deps;
}

} // namespace Internal
META_END_NAMESPACE()