find_package(GTest REQUIRED)

add_executable(antwika_atlas_editor_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/AtlasEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Canvas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/CanvasView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorUi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Palette.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PngAtlasStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SceneSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TickLimitSource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TileGrid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/UiOverlay.cpp
    CanvasTest.cpp
    CanvasViewTest.cpp
    EditorOptionsTest.cpp
    EditorSinkTest.cpp
    EditorStateTest.cpp
    EditorUiTest.cpp
    PngAtlasStoreTest.cpp
    RunIntegrationTest.cpp
    SceneTest.cpp
    TickLimitSourceTest.cpp
    TileGridTest.cpp
)

target_include_directories(antwika_atlas_editor_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_atlas_editor_tests
    PRIVATE
        antwika::app
        antwika::cli
        antwika::engine
        antwika::event::tests::mocks
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::simulation
        antwika::time::tests::fakes
        antwika::ui
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_atlas_editor_tests)
