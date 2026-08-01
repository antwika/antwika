add_executable(antwika_sudoku
    src/Board.cpp
    src/Puzzle.cpp
    src/main.cpp
)

target_include_directories(antwika_sudoku
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_sudoku
    PRIVATE
        antwika::i18n
        antwika::wfc
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_sudoku
)

install(TARGETS antwika_sudoku
    RUNTIME DESTINATION bin
)
