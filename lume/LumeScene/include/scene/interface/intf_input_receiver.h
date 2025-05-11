#ifndef SCENE_INTERFACE_IINPUT_RECEIVER_H
#define SCENE_INTERFACE_IINPUT_RECEIVER_H

#include <scene/base/types.h>

#include <meta/base/interface_macros.h>
#include <meta/base/time_span.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief The PointerEvent class defines a set of pointer events
 */
struct PointerEvent {
    using PointerId = uint32_t;

    /// Pointer state
    enum class PointerState : uint8_t {
        /// The pointer is not pressed.
        POINTER_UP = 0,
        /// The pointer is pressed.
        POINTER_DOWN,
    };
    /// A struct representing one input pointer
    struct Pointer {
        /// Id of the pointer
        PointerId id;
        /// Normalized input position between [0,0]..[1,1] in view space.
        BASE_NS::Math::Vec2 position;
        /// Pointer state
        PointerState state { PointerState::POINTER_UP };
    };
    /// List of pointer ids that changed as a result of this event
    BASE_NS::vector<Pointer> pointers;
    /// Event timestamp
    META_NS::TimeSpan time { META_NS::TimeSpan::Zero() };
    /// If true after receiver handling, no further IInputReceivers should receive the event.
    bool handled {};
};

/**
 * @brief The IInputReceiver interface can be implemented by attachments that want to receive input events.
 * @see ICamera::SendInputEvent
 */
class IInputReceiver : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInputReceiver, "17390f1d-e8d9-46b2-a8e3-661832f5dc5b")
public:
    /**
     * @brief Invoked when an input event has been sent.
     * @see ICamera::SendInputEvent
     * @param event The input event information.
     */
    virtual void OnInput(PointerEvent& event) = 0;
};

SCENE_END_NAMESPACE()

#endif // SCENE_INTERFACE_IINPUT_RECEIVER_H
