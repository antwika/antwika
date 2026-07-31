find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_rng_tests
    FakeRngTest.cpp
    SplitMix64RngTest.cpp
)

target_link_libraries(antwika_rng_tests
    PRIVATE
        antwika::rng
        antwika::rng::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_rng_tests)
