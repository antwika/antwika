add_library(event_mocks INTERFACE)

target_include_directories(event_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(event_mocks
    INTERFACE
        event
)

install(
    TARGETS event_mocks
    EXPORT eventTargets
    PUBLIC_HEADER DESTINATION include
)
