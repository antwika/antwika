find_package(GTest REQUIRED)

add_executable(antwika_tilemap_tests
    CellTest.cpp
    EntitiesTest.cpp
    ExpandedMapTest.cpp
    FlowDirectionTest.cpp
    MapHeaderTest.cpp
    MapJsonTest.cpp
    OverlayTest.cpp
    RgbTest.cpp
    TerrainClassTest.cpp
    TileMapErrorTest.cpp
    TileMapTest.cpp
    WaterAttributesTest.cpp
)

target_link_libraries(antwika_tilemap_tests
    PRIVATE
        antwika::tilemap
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_tilemap_tests)
