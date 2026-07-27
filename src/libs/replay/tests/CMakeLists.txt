find_package(GTest REQUIRED)

add_executable(antwika_replay_tests
    BinaryEventCodecTest.cpp
)

target_link_libraries(antwika_replay_tests
    PRIVATE
        antwika::replay
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_replay_tests)
