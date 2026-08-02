add_executable(antwika_atlas_editor
    src/AtlasEditor.cpp
    src/Canvas.cpp
    src/CanvasView.cpp
    src/EditorOptions.cpp
    src/EditorScene.cpp
    src/EditorSink.cpp
    src/EditorState.cpp
    src/EditorUi.cpp
    src/Messages.cpp
    src/Palette.cpp
    src/PngAtlasStore.cpp
    src/RenderSink.cpp
    src/SceneSnapshot.cpp
    src/TickLimitSource.cpp
    src/TileGrid.cpp
    src/Tool.cpp
    src/UiOverlay.cpp
    src/main.cpp
)

target_include_directories(antwika_atlas_editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_atlas_editor
    PRIVATE
        antwika::app
        antwika::cli
        antwika::engine
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
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_atlas_editor
)

install(TARGETS antwika_atlas_editor
    RUNTIME DESTINATION bin
)
