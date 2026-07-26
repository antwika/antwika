add_library(antwika_event_tests_mocks INTERFACE)

add_library(antwika::event::tests::mocks ALIAS antwika_event_tests_mocks)

target_include_directories(antwika_event_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_event_tests_mocks
    INTERFACE
        antwika::event
)

install(
    TARGETS antwika_event_tests_mocks
    EXPORT antwika_eventTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
