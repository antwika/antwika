find_package(GTest REQUIRED)

add_executable(antwika_atlas_tests
    AtlasErrorTest.cpp
    AtlasMetaFileTest.cpp
    AtlasMetaTest.cpp
)

target_link_libraries(antwika_atlas_tests
    PRIVATE
        antwika::atlas
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_atlas_tests)
