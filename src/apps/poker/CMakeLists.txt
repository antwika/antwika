add_executable(antwika_poker
    src/AgentStyle.cpp
    src/BankrollLedger.cpp
    src/CashGame.cpp
    src/PokerRoom.cpp
    src/PokerRoomSink.cpp
    src/PolicyAgent.cpp
    src/TablePrinter.cpp
    src/TableRenderSink.cpp
    src/TableScene.cpp
    src/TableSnapshot.cpp
    src/WatchOptions.cpp
    src/WindowCloseSource.cpp
    src/main.cpp
)

# Baked in at configure time, as apps/game does with its atlas, so the
# binary needs no install step and no working directory.
# Only the app gets this: every test draws through a mock instead.
set(ANTWIKA_POKER_ATLAS_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/atlas.png")

target_compile_definitions(antwika_poker
    PRIVATE
        ANTWIKA_POKER_ATLAS_PATH="${ANTWIKA_POKER_ATLAS_PATH}"
)

target_include_directories(antwika_poker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(ANTWIKA_POKER_DEMO_REPLAY_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/replays/demo.json")

target_compile_definitions(antwika_poker
    PRIVATE
        ANTWIKA_POKER_DEMO_REPLAY_PATH="${ANTWIKA_POKER_DEMO_REPLAY_PATH}"
)

target_link_libraries(antwika_poker
    PRIVATE
        antwika::app
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::gfx_backend
        antwika::holdem
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
            add_custom_command(TARGET antwika_poker POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DLL}
                    $<TARGET_FILE_DIR:antwika_poker>
            )
        endif()
    endforeach()
endif()

install(TARGETS antwika_poker
    RUNTIME DESTINATION bin
)
