#include "speed.h"

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

bool SpeedModifier::Build(const IMetadata::Ptr& data)
{
    return speedFactorChanged_.Subscribe(
        META_ACCESS_PROPERTY(SpeedFactor), interface_pointer_cast<IOnChanged>(META_ACCESS_EVENT(OnChanged).GetEvent()));
}

bool SpeedModifier::ProcessOnGetDuration(DurationData& duration) const
{
    auto speed = BASE_NS::Math::abs(META_ACCESS_PROPERTY_VALUE(SpeedFactor));
    if (speed > 0.f) {
        duration.duration = duration.duration / speed;
    } else {
        duration.duration = TimeSpan::Infinite();
    }
    return true;
}
bool SpeedModifier::ProcessOnStep(StepData& step) const
{
    auto speed = BASE_NS::Math::abs(META_ACCESS_PROPERTY_VALUE(SpeedFactor));
    if (speed < 0) {
        step.progress = 1.f - step.progress;
        step.reverse = !step.reverse;
    }
    return true;
}

} // namespace AnimationModifiers

META_END_NAMESPACE()
