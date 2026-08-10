find_package(GTest REQUIRED)

add_executable(antwika_tower_defence_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Battle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BattleScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BattleSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Campaign.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/CampaignSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ConfigFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/FileScoreStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/GridLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/HighScore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LevelGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LevelTile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Messages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/MobKind.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ScoreOverlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ScoreSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/StateDump.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TowerDefence.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TowerDefenceSnapshotStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TowerPlacementSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Wave.cpp
    BattleDrawTest.cpp
    BattleTest.cpp
    CampaignTest.cpp
    ConfigFileTest.cpp
    ConsoleSinkTest.cpp
    GridLayoutTest.cpp
    HighScoreTest.cpp
    LevelGeneratorTest.cpp
    LevelTileTest.cpp
    MessagesTest.cpp
    ReplayRoundTripTest.cpp
    RunIntegrationTest.cpp
    SceneTest.cpp
    SinksTest.cpp
    StateDumpTest.cpp
    WaveTest.cpp
)

target_include_directories(antwika_tower_defence_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_tower_defence_tests
    PRIVATE
        antwika::app
        antwika::config
        antwika::config::tests::conformance
        antwika::console
        antwika::console::tests::conformance
        antwika::engine
        antwika::event::tests::mocks
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::i18n
        antwika::i18n::tests::conformance
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::rng
        antwika::testing
        antwika::time::tests::fakes
        antwika::ui
        antwika::wfc
        gtest::gtest
)

if(ENABLE_COVERAGE)
    set(antwika_level_seed_count 8)
    set(antwika_level_property_seed_count 2)
    set(antwika_campaign_seed_count 1)
else()
    set(antwika_level_seed_count 40)
    set(antwika_level_property_seed_count 40)
    set(antwika_campaign_seed_count 8)
endif()

target_compile_definitions(antwika_tower_defence_tests
    PRIVATE
        ANTWIKA_LEVEL_SEED_COUNT=${antwika_level_seed_count}
        ANTWIKA_LEVEL_PROPERTY_SEED_COUNT=${antwika_level_property_seed_count}
        ANTWIKA_CAMPAIGN_SEED_COUNT=${antwika_campaign_seed_count}
)

antwika_bundle_test(TARGET antwika_tower_defence_tests)
