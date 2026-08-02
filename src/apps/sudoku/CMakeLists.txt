add_executable(antwika_sudoku
    src/Board.cpp
    src/BoardLayout.cpp
    src/BoardOverlay.cpp
    src/BoardSink.cpp
    src/KeyMapping.cpp
    src/PlaySink.cpp
    src/Puzzle.cpp
    src/PuzzleFile.cpp
    src/PuzzleSource.cpp
    src/PuzzleState.cpp
    src/RenderSink.cpp
    src/Solve.cpp
    src/Sudoku.cpp
    src/SudokuOptions.cpp
    src/SudokuScene.cpp
    src/TickLimitSource.cpp
    src/main.cpp
)

target_include_directories(antwika_sudoku
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_sudoku
    PRIVATE
        antwika::app
        antwika::cli
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::gfx_backend
        antwika::i18n
        antwika::input
        antwika::input_backend
        antwika::log
        antwika::replay
        antwika::simulation
        antwika::time
        antwika::ui
        antwika::wfc
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_sudoku
    ASSETS
        replays/demo.jsonl
)

install(TARGETS antwika_sudoku
    RUNTIME DESTINATION bin
)
