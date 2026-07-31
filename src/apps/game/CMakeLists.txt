add_executable(antwika_game
    src/AppMode.cpp
    src/AtlasImage.cpp
    src/BuildGhost.cpp
    src/BuildingIndex.cpp
    src/BuildingSystem.cpp
    src/Game.cpp
    src/GameStateReducer.cpp
    src/GridScene.cpp
    src/GridSink.cpp
    src/Homing.cpp
    src/InputFold.cpp
    src/MainMenuScene.cpp
    src/MainMenuSink.cpp
    src/ModeGatedSink.cpp
    src/ModeGatedSystem.cpp
    src/PathIndex.cpp
    src/Placement.cpp
    src/RenderSystem.cpp
    src/KeyText.cpp
    src/SaveCli.cpp
    src/SaveGame.cpp
    src/SaveGameFile.cpp
    src/SaveDirectory.cpp
    src/SaveLoadScene.cpp
    src/SaveLoadSink.cpp
    src/SaveLoadState.cpp
    src/SaveMigration.cpp
    src/SessionStore.cpp
    src/SpawnSystem.cpp
    src/Toolbar.cpp
    src/SceneSnapshot.cpp
    src/UiOverlay.cpp
    src/UiSink.cpp
    src/WalkerMotion.cpp
    src/WalkerSystem.cpp
    src/WorldMap.cpp
    src/WorldMapScene.cpp
    src/WorldMapSink.cpp
    src/WorldMapState.cpp
    src/WindowInputSource.cpp
    src/main.cpp
)

target_include_directories(antwika_game
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_game
    PRIVATE
        antwika::animation
        antwika::app
        antwika::ecs
        antwika::engine
        antwika::gfx
        antwika::gfx_backend
        antwika::input
        antwika::input_backend
        antwika::log
        antwika::pathfinding
        antwika::replay
        antwika::time
        antwika::ui
        antwika::wfc
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_game
    ASSETS
        assets/atlas.png
)

install(TARGETS antwika_game
    RUNTIME DESTINATION bin
)
