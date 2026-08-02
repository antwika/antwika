find_package(GTest REQUIRED)

add_executable(antwika_tower_defence_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Battle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BattleScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BattleSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Campaign.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/CampaignSink.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TowerDefence.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TowerPlacementSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Wave.cpp
    BattleTest.cpp
    CampaignTest.cpp
    GridLayoutTest.cpp
    HighScoreTest.cpp
    LevelGeneratorTest.cpp
    LevelTileTest.cpp
    MessagesTest.cpp
    ReplayRoundTripTest.cpp
    RunIntegrationTest.cpp
    SinksTest.cpp
    SceneTest.cpp
    WaveTest.cpp
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
        antwika::i18n
        antwika::i18n::tests::conformance
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::rng
        antwika::time::tests::fakes
        antwika::ui
        antwika::wfc
        gtest::gtest
)

# How many seeds the linearity property is soaked over.
# Generating one level costs ~2.9s under the coverage build's -O0 and
# ~0.14s at -O2, so the wide sweep is 116 seconds of a coverage run and
# under six of an optimised one.
# Eight seeds reach every line, function and branch of the generator --
# measured, by running the suite both ways and diffing the gcovr totals
# -- so the coverage legs prove the coverage and an optimised build
# proves the property.
# CI builds this one target against conan-release to do that; see the
# "Soak the level generator" step in .github/workflows/build.yml.
# The linearity soak is the one that walks the generator through every
# branch it has, so the cases asserting other properties of a level get
# a couple of seeds under instrumentation and the full sweep otherwise.
# The shipped campaign's own grids get a third count, since each of
# them is a whole generation and there are as many of them as there are
# levels: one seed apiece under instrumentation, eight otherwise.
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
