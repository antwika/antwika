add_library(antwika_pathfinding_tests_fakes INTERFACE)

add_library(antwika::pathfinding::tests::fakes ALIAS
    antwika_pathfinding_tests_fakes)

target_include_directories(antwika_pathfinding_tests_fakes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_pathfinding_tests_fakes
    INTERFACE
        antwika::pathfinding
)

install(
    TARGETS antwika_pathfinding_tests_fakes
    EXPORT antwika_pathfindingTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
