find_package(GTest REQUIRED)

add_executable(antwika_light_tests
    ActiveLightTest.cpp
    PointLightTest.cpp
)

target_link_libraries(antwika_light_tests
    PRIVATE
        antwika::light
        antwika::log::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_light_tests)
