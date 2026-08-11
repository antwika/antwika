find_package(GTest REQUIRED)

add_executable(antwika_tilemap_tests
    ColumnTest.cpp
    EntitiesTest.cpp
    ExpandedMapTest.cpp
    FlowDirectionTest.cpp
    MapFileTest.cpp
    MapHeaderTest.cpp
    MapJsonTest.cpp
    OverlayTest.cpp
    RgbTest.cpp
    SlabTest.cpp
    TerrainClassTest.cpp
    TileMapErrorTest.cpp
    TileMapTest.cpp
    WaterAttributesTest.cpp
)

target_link_libraries(antwika_tilemap_tests
    PRIVATE
        antwika::testing
        antwika::tilemap
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_tilemap_tests)
