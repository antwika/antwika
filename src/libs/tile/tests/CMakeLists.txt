find_package(GTest REQUIRED)

add_executable(antwika_tile_tests
    TilePaintTest.cpp
    TileRulesTest.cpp
    TileShapesTest.cpp
    TransitionsTest.cpp
)

target_link_libraries(antwika_tile_tests
    PRIVATE
        antwika::tile
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_tile_tests)
