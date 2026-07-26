add_library(antwika_log_tests_mocks INTERFACE)

add_library(antwika::log::tests::mocks ALIAS antwika_log_tests_mocks)

target_include_directories(antwika_log_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_log_tests_mocks
    INTERFACE
        antwika::log
)

install(
    TARGETS antwika_log_tests_mocks
    EXPORT antwika_logTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
