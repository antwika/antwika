find_package(GTest REQUIRED)

add_executable(antwika_rules_tests
    GatesTest.cpp
    ItemsTest.cpp
)

target_link_libraries(antwika_rules_tests
    PRIVATE
        antwika::rules
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_rules_tests)
