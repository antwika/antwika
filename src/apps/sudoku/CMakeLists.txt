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
        antwika::wfc
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

if(MINGW)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libgcc_s_seh-1.dll
        OUTPUT_VARIABLE LIBGCC_DLL
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++-6.dll
        OUTPUT_VARIABLE LIBSTDCPP_DLL
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libwinpthread-1.dll
        OUTPUT_VARIABLE LIBWINPTHREAD_DLL
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    foreach(DLL
        ${LIBGCC_DLL}
        ${LIBSTDCPP_DLL}
        ${LIBWINPTHREAD_DLL}
    )
        if(EXISTS ${DLL})
            add_custom_command(TARGET antwika_sudoku POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DLL}
                    $<TARGET_FILE_DIR:antwika_sudoku>
            )
        endif()
    endforeach()
endif()

install(TARGETS antwika_sudoku
    RUNTIME DESTINATION bin
)
