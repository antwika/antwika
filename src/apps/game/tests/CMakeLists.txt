find_package(GTest REQUIRED)

add_executable(antwika_game_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Game.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/GameStateReducer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DiamondSpans.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/GridScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/GridSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PathIndex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SceneSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/WalkerSystem.cpp
    BootstrapTest.cpp
    CameraTest.cpp
    CellTest.cpp
    DiamondSpansTest.cpp
    DirectionTest.cpp
    GameStateReducerTest.cpp
    GameStateTest.cpp
    GameTest.cpp
    GridExtentTest.cpp
    GridSceneTest.cpp
    GridSinkTest.cpp
    IsoProjectionTest.cpp
    PathIndexTest.cpp
    RenderSystemTest.cpp
    SceneSnapshotTest.cpp
    ReplayIntegrationTest.cpp
    WalkerSystemTest.cpp
    WalkerTest.cpp
    WalkingTest.cpp
)

target_include_directories(antwika_game_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_game_tests
    PRIVATE
        antwika::engine::tests::mocks
        antwika::event::tests::mocks
        antwika::ecs
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_game_tests)
