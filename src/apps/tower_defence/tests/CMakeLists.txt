find_package(GTest REQUIRED)

add_executable(antwika_tower_defence_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Battle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BattleScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BattleSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BattleSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/GridLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LevelGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LevelTile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ScoreOverlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ScoreSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TowerDefence.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TowerPlacementSink.cpp
    BattleTest.cpp
    GridLayoutTest.cpp
    LevelGeneratorTest.cpp
    LevelTileTest.cpp
    RunIntegrationTest.cpp
    SinksTest.cpp
    SceneTest.cpp
)

target_include_directories(antwika_tower_defence_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_tower_defence_tests
    PRIVATE
        antwika::engine
        antwika::event::tests::mocks
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::time::tests::fakes
        antwika::ui
        antwika::wfc
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_tower_defence_tests)
