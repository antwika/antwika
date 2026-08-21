find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_pathfinding_tests
    PathTest.cpp
)

target_link_libraries(antwika_pathfinding_tests
    PRIVATE
        antwika::pathfinding
        antwika::pathfinding::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_pathfinding_tests)
