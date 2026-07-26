find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_engine_tests
    EngineTest.cpp
)

target_link_libraries(antwika_engine_tests
    PRIVATE
        antwika::engine
        antwika::engine::tests::mocks
        antwika::log::tests::mocks
        antwika::event::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_engine_tests)
