find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_pathfinding_tests
    AStarDeterminismTest.cpp
    AStarHeuristicTest.cpp
    AStarTest.cpp
    GridGraphTest.cpp
    GridSearchTest.cpp
    NodeIdTest.cpp
)

target_link_libraries(antwika_pathfinding_tests
    PRIVATE
        antwika::pathfinding
        antwika::pathfinding::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_pathfinding_tests)
