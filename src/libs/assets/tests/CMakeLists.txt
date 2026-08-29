find_package(GTest REQUIRED)

add_executable(antwika_assets_tests
    MapAssetsTest.cpp
)

target_link_libraries(antwika_assets_tests
    PRIVATE
        antwika::assets
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_assets_tests)
