find_package(GTest REQUIRED)

add_executable(antwika_decor_tests
    DecorTest.cpp
    TileAnimationTest.cpp
    VariantsTest.cpp
)

target_link_libraries(antwika_decor_tests
    PRIVATE
        antwika::decor
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_decor_tests)
