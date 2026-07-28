add_library(antwika_ecs_tests_mocks INTERFACE)

add_library(antwika::ecs::tests::mocks ALIAS antwika_ecs_tests_mocks)

target_include_directories(antwika_ecs_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_ecs_tests_mocks
    INTERFACE
        antwika::ecs
)

install(
    TARGETS antwika_ecs_tests_mocks
    EXPORT antwika_ecsTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
