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

antwika_bundle_app(
    TARGET antwika_tower_defence
)

install(TARGETS antwika_tower_defence
    RUNTIME DESTINATION bin
)
