add_executable(antwika_tower_defence
    src/Battle.cpp
    src/BattleScene.cpp
    src/BattleSink.cpp
    src/BattleSnapshot.cpp
    src/GridLayout.cpp
    src/LevelGenerator.cpp
    src/LevelTile.cpp
    src/RenderSink.cpp
    src/ScoreOverlay.cpp
    src/ScoreSink.cpp
    src/TowerDefence.cpp
    src/TowerPlacementSink.cpp
    src/main.cpp
)

target_include_directories(antwika_tower_defence
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_tower_defence
    PRIVATE
        antwika::app
        antwika::engine
        antwika::gfx
        antwika::gfx_backend
        antwika::input
        antwika::input_backend
        antwika::log
        antwika::replay
        antwika::time
        antwika::ui
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
            add_custom_command(TARGET antwika_tower_defence POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DLL}
                    $<TARGET_FILE_DIR:antwika_tower_defence>
            )
        endif()
    endforeach()
endif()

install(TARGETS antwika_tower_defence
    RUNTIME DESTINATION bin
)
