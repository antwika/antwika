find_package(GTest REQUIRED)

add_executable(antwika_tilemap_tests
    TilemapTest.cpp
)

target_link_libraries(antwika_tilemap_tests
    PRIVATE
        antwika::tilemap
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_tilemap_tests)
