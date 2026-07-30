find_package(GTest REQUIRED)

add_executable(antwika_poker_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/AgentStyle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BankrollLedger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/CashGame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PokerRoom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PokerRoomSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PolicyAgent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TablePrinter.cpp
    AgentStyleTest.cpp
    BankrollLedgerTest.cpp
    BootstrapTest.cpp
    CashGameTest.cpp
    PokerRoomSinkTest.cpp
    PolicyAgentTest.cpp
    ReplayIntegrationTest.cpp
    RoomSummaryTest.cpp
    TablePrinterTest.cpp
)

target_include_directories(antwika_poker_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_poker_tests
    PRIVATE
        antwika::engine
        antwika::event
        antwika::holdem
        antwika::holdem::tests::fakes
        antwika::log
        antwika::log::tests::mocks
        antwika::replay
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_poker_tests)
