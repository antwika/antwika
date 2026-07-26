add_library(antwika_time_tests_fakes INTERFACE)

add_library(antwika::time::tests::fakes ALIAS antwika_time_tests_fakes)

target_include_directories(antwika_time_tests_fakes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_time_tests_fakes
    INTERFACE
        antwika_time
)

install(
    TARGETS antwika_time_tests_fakes
    EXPORT antwika_timeTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
