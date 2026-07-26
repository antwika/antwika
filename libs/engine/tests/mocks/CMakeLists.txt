add_library(engine_mocks INTERFACE)

target_include_directories(engine_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine_mocks
    INTERFACE
        engine
)

install(
    TARGETS engine_mocks
    EXPORT engineTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
