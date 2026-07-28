find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_reducer_tests
    ReducerSinkTest.cpp
)

target_link_libraries(antwika_reducer_tests
    PRIVATE
        antwika::reducer
        antwika::reducer::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_reducer_tests)
