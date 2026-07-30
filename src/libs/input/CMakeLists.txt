# antwika::replay comes in for the replay bridge alone: IReplaySource is
# the seam live input travels through, and PayloadJson is the shared
# parse-and-validate step every structured payload already uses.
# antwika::engine comes in for StopOnKeySource, which names engine.stop.
# antwika::gfx is deliberately absent, and stays absent -- reading input
# does not require opening a window.
antwika_add_library(
    NAME input
    SOURCES
        src/ActionMap.cpp
        src/CoalescingPointerSource.cpp
        src/IdleMotionSource.cpp
        src/InputEventCodec.cpp
        src/InputPipeline.cpp
        src/InputState.cpp
        src/Key.cpp
        src/Keyboard.cpp
        src/LiveInputSource.cpp
        src/Mouse.cpp
        src/MouseButton.cpp
        src/NullInputBackend.cpp
        src/StopOnKeySource.cpp
    DEPENDS
        antwika::engine
        antwika::event
        antwika::log
        antwika::replay
        antwika::time
)
