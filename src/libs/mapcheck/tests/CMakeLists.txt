find_package(GTest REQUIRED)

add_executable(antwika_mapcheck_tests
    ValidateTest.cpp
)

target_link_libraries(antwika_mapcheck_tests
    PRIVATE
        antwika::mapcheck
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_mapcheck_tests)
