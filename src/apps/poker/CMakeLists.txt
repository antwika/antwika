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
    src/main.cpp
)

target_include_directories(antwika_poker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_poker
    PRIVATE
        antwika::app
        antwika::cli
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::gfx_backend
        antwika::holdem
        antwika::log
        antwika::replay
        antwika::rng
        antwika::simulation
        antwika::time
        antwika::ui
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_poker
    ASSETS
        assets/atlas.png
        replays/demo.jsonl
)

install(TARGETS antwika_poker
    RUNTIME DESTINATION bin
)
