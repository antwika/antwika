find_package(GTest REQUIRED)

add_executable(antwika_life_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Board.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BoardLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BoardScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BoardSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DragPausedSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DragState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Grid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Life.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LifeSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PointerToggleSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PrintSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSystem.cpp
    BoardLayoutTest.cpp
    BoardSceneTest.cpp
    BoardSinkTest.cpp
    BoardTest.cpp
    BootstrapTest.cpp
    DragPausedSystemTest.cpp
    DragStateTest.cpp
    GridTest.cpp
    LifeSystemTest.cpp
    PointerReplayIntegrationTest.cpp
    PointerToggleSinkTest.cpp
    PrintSystemTest.cpp
    RenderDeterminismTest.cpp
    RenderSystemTest.cpp
    ReplayIntegrationTest.cpp
)

target_include_directories(antwika_life_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_life_tests
    PRIVATE
        antwika::ecs
        antwika::ecs::tests::mocks
        antwika::engine
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::input::tests::fakes
        antwika::log::tests::mocks
        antwika::replay
        antwika::simulation
        antwika::time::tests::fakes
        antwika::event::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_life_tests)
