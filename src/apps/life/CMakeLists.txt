add_executable(antwika_life
    src/Board.cpp
    src/BoardLayout.cpp
    src/BoardScene.cpp
    src/BoardSink.cpp
    src/DragPausedSystem.cpp
    src/DragState.cpp
    src/Grid.cpp
    src/Life.cpp
    src/LifeSystem.cpp
    src/PointerToggleSink.cpp
    src/PrintSystem.cpp
    src/RenderSystem.cpp
    src/TickPacer.cpp
    src/WindowInputSource.cpp
    src/main.cpp
)

target_include_directories(antwika_life
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_life
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
        antwika::simulation
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_life
    ASSETS
        replays/demo.json
)

install(TARGETS antwika_life
    RUNTIME DESTINATION bin
)
