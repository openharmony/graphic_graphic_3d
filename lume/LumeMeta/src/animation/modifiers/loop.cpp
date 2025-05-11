#include "loop.h"

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/builtin_objects.h>

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

bool LoopModifier::Build(const IMetadata::Ptr& data)
{
    LoopCount()->OnChanged()->AddHandler(META_ACCESS_EVENT(OnChanged).GetCallable());
    return true;
}

bool LoopModifier::ProcessOnGetDuration(DurationData& duration) const
{
    const auto loopCount = META_ACCESS_PROPERTY_VALUE(LoopCount);
    if (loopCount == 0) {
        // Invalid
        return false;
    }
    if (loopCount > 0) {
        duration.loopCount = duration.loopCount < 0 ? duration.loopCount : duration.loopCount + loopCount - 1;
    } else {
        duration.loopCount = -1;
    }
    return true;
}

bool LoopModifier::ProcessOnStep(StepData& step) const
{
    return true;
}

} // namespace AnimationModifiers

META_END_NAMESPACE()
