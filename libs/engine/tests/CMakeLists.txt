find_package(GTest REQUIRED)

add_executable(engine_tests
    EngineTest.cpp
)

target_link_libraries(engine_tests
    PRIVATE
        antwika::engine
        log_mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(engine_tests)
