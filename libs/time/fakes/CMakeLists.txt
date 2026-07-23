add_library(time_fakes INTERFACE)

target_include_directories(time_fakes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(time_fakes
    INTERFACE
        time
)

install(
    TARGETS time_fakes
    EXPORT timeTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
