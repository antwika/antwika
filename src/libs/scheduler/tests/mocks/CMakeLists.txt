add_library(antwika_scheduler_tests_mocks INTERFACE)

add_library(
    antwika::scheduler::tests::mocks
    ALIAS antwika_scheduler_tests_mocks
)

target_include_directories(antwika_scheduler_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_scheduler_tests_mocks
    INTERFACE
        antwika::scheduler
)

install(
    TARGETS antwika_scheduler_tests_mocks
    EXPORT antwika_schedulerTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
