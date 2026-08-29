find_package(GTest REQUIRED)

add_executable(antwika_testing_tests
    ScratchPathTest.cpp
)

target_link_libraries(antwika_testing_tests
    PRIVATE
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_testing_tests)
