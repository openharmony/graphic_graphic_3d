#include "reverse.h"

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

bool ReverseModifier::Build(const IMetadata::Ptr& data)
{
    return true;
}

bool ReverseModifier::ProcessOnGetDuration(DurationData& duration) const
{
    return true;
}
bool ReverseModifier::ProcessOnStep(StepData& step) const
{
    step.progress = 1.f - step.progress;
    step.reverse = !step.reverse;
    return true;
}

} // namespace AnimationModifiers

META_END_NAMESPACE()
