add_library(antwika_input
    src/ActionMap.cpp
    src/CoalescingPointerSource.cpp
    src/InputEventCodec.cpp
    src/InputState.cpp
    src/Key.cpp
    src/Keyboard.cpp
    src/LiveInputSource.cpp
    src/Mouse.cpp
    src/MouseButton.cpp
    src/NullInputBackend.cpp
    src/StopOnKeySource.cpp
)

add_library(antwika::input ALIAS antwika_input)

set_target_properties(antwika_input PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_input
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# antwika::replay comes in for the replay bridge alone: IReplaySource is
# the seam live input travels through, and PayloadJson is the shared
# parse-and-validate step every structured payload already uses.
# antwika::engine comes in for StopOnKeySource, which names engine.stop.
# antwika::gfx is deliberately absent, and stays absent -- reading input
# does not require opening a window.
target_link_libraries(antwika_input
    PUBLIC
        antwika::engine
        antwika::event
        antwika::log
        antwika::replay
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_input
    EXPORT antwika_inputTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_inputTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
