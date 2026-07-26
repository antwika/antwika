add_library(log_mocks INTERFACE)

target_include_directories(log_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(log_mocks
    INTERFACE
        log
)

install(
    TARGETS log_mocks
    EXPORT logTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
