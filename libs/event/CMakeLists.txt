cmake_minimum_required(VERSION 3.28)

project(event CXX)

add_library(event
    src/antwika/event/EventDispatcher.cpp
    src/antwika/event/EventQueue.cpp
    src/antwika/event/EventRecorder.cpp
)

add_library(antwika::event ALIAS event)

set_target_properties(event PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

set(EVENT_PUBLIC_HEADERS
    include/antwika/event/Event.hpp
    include/antwika/event/EventDispatcher.hpp
    include/antwika/event/EventQueue.hpp
    include/antwika/event/EventRecorder.hpp
    include/antwika/event/IEventDispatcher.hpp
    include/antwika/event/IEventHistory.hpp
    include/antwika/event/IEventQueue.hpp
    include/antwika/event/IEventRecorder.hpp
    include/antwika/event/IEventSink.hpp
)

set_target_properties(event PROPERTIES
    PUBLIC_HEADER "${EVENT_PUBLIC_HEADERS}"
)

target_include_directories(event
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_subdirectory(mocks)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS event
    EXPORT eventTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT eventTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/event
)
