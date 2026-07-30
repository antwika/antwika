find_package(GTest REQUIRED)

add_subdirectory(fakes)
add_subdirectory(mocks)

add_executable(antwika_holdem_tests
    ActionTest.cpp
    ActionTypeTest.cpp
    BlindsTest.cpp
    CardTest.cpp
    CardTextTest.cpp
    DeckTest.cpp
    HandCategoryTest.cpp
    HandEvaluatorTest.cpp
    HandValueTest.cpp
    PayoutTest.cpp
    PotsTest.cpp
    SplitMix64RngTest.cpp
    StageTest.cpp
    TableBettingTest.cpp
    TableRunnerTest.cpp
    TableSeatingTest.cpp
    TableShowdownTest.cpp
    TableStageTest.cpp
)

target_include_directories(antwika_holdem_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_holdem_tests
    PRIVATE
        antwika::holdem
        antwika::holdem::tests::fakes
        antwika::holdem::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_holdem_tests)
