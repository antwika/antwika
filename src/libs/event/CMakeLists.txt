add_library(antwika_event
    src/EventDispatcher.cpp
    src/EventQueue.cpp
    src/EventRecorder.cpp
    src/ReplayRecorder.cpp
    src/TickedEventDispatcher.cpp
)

add_library(antwika::event ALIAS antwika_event)

set_target_properties(antwika_event PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_event
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_event
    PUBLIC
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_event
    EXPORT antwika_eventTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_eventTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
