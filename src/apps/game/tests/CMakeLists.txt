find_package(GTest REQUIRED)

add_executable(antwika_game_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Game.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/GameStateReducer.cpp
    BootstrapTest.cpp
    GameStateReducerTest.cpp
    GameTest.cpp
)

target_include_directories(antwika_game_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_game_tests
    PRIVATE
        antwika::engine::tests::mocks
        antwika::event::tests::mocks
        antwika::log::tests::mocks
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_game_tests)
