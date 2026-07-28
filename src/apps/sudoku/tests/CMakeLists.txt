find_package(GTest REQUIRED)

add_executable(antwika_sudoku_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Board.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Puzzle.cpp
    BoardTest.cpp
    PuzzleTest.cpp
    SudokuSolverIntegrationTest.cpp
    UnsolvablePuzzleTest.cpp
    SudokuDeterminismTest.cpp
)

target_include_directories(antwika_sudoku_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_sudoku_tests
    PRIVATE
        antwika::wfc
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_sudoku_tests)
