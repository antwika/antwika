add_executable(antwika_game
    src/Game.cpp
    src/GameStateReducer.cpp
    src/GridScene.cpp
    src/GridSink.cpp
    src/InputFold.cpp
    src/PathIndex.cpp
    src/RenderSystem.cpp
    src/TickPacer.cpp
    src/Toolbar.cpp
    src/SceneSnapshot.cpp
    src/UiOverlay.cpp
    src/UiSink.cpp
    src/WalkerSystem.cpp
    src/WindowInputSource.cpp
    src/main.cpp
)

# Baked in at configure time, as apps/gfx_demo does with its logo, so the
# binary needs no install step and no working directory.
# Only the app gets this: every test draws a mock texture instead.
set(ANTWIKA_GAME_ATLAS_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/atlas.png")

target_compile_definitions(antwika_game
    PRIVATE
        ANTWIKA_GAME_ATLAS_PATH="${ANTWIKA_GAME_ATLAS_PATH}"
)

target_include_directories(antwika_game
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_game
    PRIVATE
        antwika::app
        antwika::ecs
        antwika::engine
        antwika::gfx
        antwika::gfx_backend
        antwika::input
        antwika::input_backend
        antwika::log
        antwika::replay
        antwika::time
        antwika::ui
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
            add_custom_command(TARGET antwika_game POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DLL}
                    $<TARGET_FILE_DIR:antwika_game>
            )
        endif()
    endforeach()
endif()

install(TARGETS antwika_game
    RUNTIME DESTINATION bin
)
