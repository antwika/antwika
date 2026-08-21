find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_worldgen_tests
    ChunkClimbTest.cpp
    ChunkShapeTest.cpp
    CityRulesetTest.cpp
    ExpandTest.cpp
    GrowChunkTest.cpp
    RulesetTest.cpp
    StairsTest.cpp
)

target_include_directories(antwika_worldgen_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_worldgen_tests
    PRIVATE
        antwika::worldgen
        antwika::pathfinding
        antwika::worldgen::tests::fakes
        antwika::rng::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_worldgen_tests)
