find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_pathfinding_tests
    NodeIdTest.cpp
    AStarTest.cpp
    AStarDeterminismTest.cpp
    AStarHeuristicTest.cpp
    GridGraphTest.cpp
    GridSearchTest.cpp
)

target_link_libraries(antwika_pathfinding_tests
    PRIVATE
        antwika::pathfinding
        antwika::pathfinding::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_pathfinding_tests)
