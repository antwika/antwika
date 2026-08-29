find_package(GTest REQUIRED)

add_executable(antwika_component_tests
    EqualityTest.cpp
)

target_link_libraries(antwika_component_tests
    PRIVATE
        antwika::component
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_component_tests)
