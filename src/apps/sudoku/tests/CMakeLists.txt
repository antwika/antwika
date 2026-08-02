find_package(GTest REQUIRED)

add_executable(antwika_sudoku_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Board.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BoardLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BoardOverlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BoardSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/KeyMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Messages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PlaySink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Puzzle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PuzzleFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PuzzleSource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PuzzleState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Solve.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sudoku.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SudokuOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SudokuScene.cpp
    BoardLayoutTest.cpp
    BoardSinkTest.cpp
    BoardTest.cpp
    KeyMappingTest.cpp
    MessagesTest.cpp
    PlaySinkTest.cpp
    PuzzleFileTest.cpp
    PuzzleSourceTest.cpp
    PuzzleStateTest.cpp
    PuzzleTest.cpp
    RenderSinkTest.cpp
    ReplayIntegrationTest.cpp
    SolveTest.cpp
    SudokuDeterminismTest.cpp
    SudokuOptionsTest.cpp
    SudokuSceneTest.cpp
    SudokuSolverIntegrationTest.cpp
    UnsolvablePuzzleTest.cpp
)

target_include_directories(antwika_sudoku_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_sudoku_tests
    PRIVATE
        antwika::app
        antwika::cli
        antwika::engine
        antwika::event
        antwika::event::tests::mocks
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::i18n
        antwika::i18n::tests::conformance
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::simulation
        antwika::time::tests::fakes
        antwika::ui
        antwika::wfc
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_sudoku_tests)
