find_package(GTest REQUIRED)

add_executable(antwika_tileset_tests
    AtlasTest.cpp
    PixelClassTest.cpp
    SpriteTest.cpp
    TilesetErrorTest.cpp
    TilesetFileTest.cpp
    TilesetJsonTest.cpp
    TilesetTest.cpp
)

target_link_libraries(antwika_tileset_tests
    PRIVATE
        antwika::testing
        antwika::tileset
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_tileset_tests)
